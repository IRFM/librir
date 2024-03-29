// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <jpegls_error.h>

#include "jpeg_marker_code.h"
#include "process_line.h"
#include "util.h"

#include <cassert>
#include <memory>

namespace charls
{

    // Purpose: Implements encoding to stream of bits. In encoding mode JpegLsCodec inherits from EncoderStrategy
    class decoder_strategy
    {
    public:
        explicit decoder_strategy(const frame_info &frame, const coding_parameters &parameters) noexcept : frame_info_{frame},
                                                                                                           parameters_{parameters}
        {
        }

        virtual ~decoder_strategy() = default;

        decoder_strategy(const decoder_strategy &) = delete;
        decoder_strategy(decoder_strategy &&) = delete;
        decoder_strategy &operator=(const decoder_strategy &) = delete;
        decoder_strategy &operator=(decoder_strategy &&) = delete;

        virtual std::unique_ptr<process_line> create_process_line(byte_stream_info raw_stream_info, uint32_t stride) = 0;
        virtual void set_presets(const jpegls_pc_parameters &preset_coding_parameters) = 0;
        virtual void decode_scan(std::unique_ptr<process_line> output_data, const JlsRect &size, byte_stream_info &compressed_data) = 0;

        void initialize(byte_stream_info &compressed_stream)
        {
            valid_bits_ = 0;
            read_cache_ = 0;

            if (compressed_stream.rawStream)
            {
                buffer_.resize(40000);
                position_ = buffer_.data();
                end_position_ = position_;
                byte_stream_ = compressed_stream.rawStream;
                add_bytes_from_stream();
            }
            else
            {
                byte_stream_ = nullptr;
                position_ = compressed_stream.rawData;
                end_position_ = position_ + compressed_stream.count;
            }

            next_ff_position_ = find_next_ff();
            make_valid();
        }

        void add_bytes_from_stream()
        {
            if (!byte_stream_ || byte_stream_->sgetc() == std::char_traits<char>::eof())
                return;

            const auto count = end_position_ - position_;

            if (count > 64)
                return;

            for (std::size_t i = 0; i < static_cast<std::size_t>(count); ++i)
            {
                buffer_[i] = position_[i];
            }
            const auto offset = buffer_.data() - position_;

            position_ += offset;
            end_position_ += offset;
            next_ff_position_ += offset;

            const std::streamsize read_bytes = byte_stream_->sgetn(reinterpret_cast<char *>(end_position_),
                                                                   static_cast<std::streamsize>(buffer_.size()) - count);
            end_position_ += read_bytes;
        }

        FORCE_INLINE void skip(const int32_t length) noexcept
        {
            valid_bits_ -= length;
            read_cache_ = read_cache_ << length;
        }

        static void on_line_begin(const size_t /*pixel_count*/, void * /*ptypeBuffer*/, int32_t /*pixelStride*/) noexcept
        {
        }

        void on_line_end(const size_t pixel_count, const void *source, const int32_t pixel_stride) const
        {
            process_line_->new_line_decoded(source, pixel_count, pixel_stride);
        }

        void end_scan()
        {
            if (*position_ != jpeg_marker_start_byte)
            {
                read_bit();

                if (*position_ != jpeg_marker_start_byte)
                    impl::throw_jpegls_error(jpegls_errc::too_much_encoded_data);
            }

            if (read_cache_ != 0)
                impl::throw_jpegls_error(jpegls_errc::too_much_encoded_data);
        }

        FORCE_INLINE bool optimized_read() noexcept
        {
            // Easy & fast: if there is no 0xFF byte in sight, we can read without bit stuffing
            if (position_ < next_ff_position_ - (sizeof(bufType) - 1))
            {
                read_cache_ |= from_big_endian<sizeof(bufType)>::read(position_) >> valid_bits_;
                const int bytes_to_read = (bufType_bit_count - valid_bits_) >> 3;
                position_ += bytes_to_read;
                valid_bits_ += bytes_to_read * 8;
                ASSERT(valid_bits_ >= bufType_bit_count - 8);
                return true;
            }
            return false;
        }

        void make_valid()
        {
            ASSERT(valid_bits_ <= bufType_bit_count - 8);

            if (optimized_read())
                return;

            add_bytes_from_stream();

            do
            {
                if (position_ >= end_position_)
                {
                    if (valid_bits_ <= 0)
                        impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);

                    return;
                }

                const bufType value_new = position_[0];

                if (value_new == jpeg_marker_start_byte)
                {
                    // JPEG bit stream rule: no FF may be followed by 0x80 or higher
                    if (position_ == end_position_ - 1 || (position_[1] & 0x80) != 0)
                    {
                        if (valid_bits_ <= 0)
                            impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);

                        return;
                    }
                }

                read_cache_ |= value_new << (bufType_bit_count - 8 - valid_bits_);
                position_ += 1;
                valid_bits_ += 8;

                if (value_new == jpeg_marker_start_byte)
                {
                    --valid_bits_;
                }
            } while (valid_bits_ < bufType_bit_count - 8);

            next_ff_position_ = find_next_ff();
        }

        uint8_t *find_next_ff() const noexcept
        {
            auto *position_next_ff = position_;

            while (position_next_ff < end_position_)
            {
                if (*position_next_ff == jpeg_marker_start_byte)
                    break;

                ++position_next_ff;
            }

            return position_next_ff;
        }

        uint8_t *get_cur_byte_pos() const noexcept
        {
            int32_t valid_bits = valid_bits_;
            uint8_t *compressed_bytes = position_;

            for (;;)
            {
                const int32_t last_bits_count = compressed_bytes[-1] == jpeg_marker_start_byte ? 7 : 8;

                if (valid_bits < last_bits_count)
                    return compressed_bytes;

                valid_bits -= last_bits_count;
                --compressed_bytes;
            }
        }

        FORCE_INLINE int32_t read_value(const int32_t length)
        {
            if (valid_bits_ < length)
            {
                make_valid();
                if (valid_bits_ < length)
                    impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);
            }

            ASSERT(length != 0 && length <= valid_bits_);
            ASSERT(length < 32);
            const auto result = static_cast<int32_t>(read_cache_ >> (bufType_bit_count - length));
            skip(length);
            return result;
        }

        FORCE_INLINE int32_t peek_byte()
        {
            if (valid_bits_ < 8)
            {
                make_valid();
            }

            return static_cast<int32_t>(read_cache_ >> (bufType_bit_count - 8));
        }

        FORCE_INLINE bool read_bit()
        {
            if (valid_bits_ <= 0)
            {
                make_valid();
            }

            const bool set = (read_cache_ & (static_cast<bufType>(1) << (bufType_bit_count - 1))) != 0;
            skip(1);
            return set;
        }

        FORCE_INLINE int32_t peek_0_bits()
        {
            if (valid_bits_ < 16)
            {
                make_valid();
            }
            bufType val_test = read_cache_;

            for (int32_t count = 0; count < 16; ++count)
            {
                if ((val_test & (static_cast<bufType>(1) << (bufType_bit_count - 1))) != 0)
                    return count;

                val_test <<= 1;
            }
            return -1;
        }

        FORCE_INLINE int32_t read_high_bits()
        {
            const int32_t count = peek_0_bits();
            if (count >= 0)
            {
                skip(count + 1);
                return count;
            }
            skip(15);

            for (int32_t high_bits_count = 15;; ++high_bits_count)
            {
                if (read_bit())
                    return high_bits_count;
            }
        }

        int32_t read_long_value(const int32_t length)
        {
            if (length <= 24)
                return read_value(length);

            return (read_value(length - 24) << 24) + read_value(24);
        }

    protected:
        frame_info frame_info_;
        coding_parameters parameters_;
        std::unique_ptr<process_line> process_line_;

    private:
        using bufType = std::size_t;
        static constexpr auto bufType_bit_count = static_cast<int32_t>(sizeof(bufType) * 8);

        std::vector<uint8_t> buffer_;
        std::basic_streambuf<char> *byte_stream_{};

        // decoding
        bufType read_cache_{};
        int32_t valid_bits_{};
        uint8_t *position_{};
        uint8_t *next_ff_position_{};
        uint8_t *end_position_{};
    };

} // namespace charls
