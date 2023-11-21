// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls_legacy.h>
#include <jpegls_error.h>

#include "coding_parameters.h"
#include "util.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <vector>

//
// This file defines the ProcessLine base class, its derivatives and helper functions.
// During coding/decoding, CharLS process one line at a time. The different ProcessLine implementations
// convert the uncompressed format to and from the internal format for encoding.
// Conversions include color transforms, line interleaved vs sample interleaved, masking out unused bits,
// accounting for line padding etc.
// This mechanism could be used to encode/decode images as they are received.
//

namespace charls
{

    class process_line
    {
    public:
        virtual ~process_line() = default;

        process_line(const process_line &) = delete;
        process_line(process_line &&) = delete;
        process_line &operator=(const process_line &) = delete;
        process_line &operator=(process_line &&) = delete;

        virtual void new_line_decoded(const void *source, size_t pixel_count, int source_stride) = 0;
        virtual void new_line_requested(void *destination, size_t pixel_count, int destination_stride) = 0;

    protected:
        process_line() = default;
    };

    class post_process_single_component final : public process_line
    {
    public:
        post_process_single_component(void *raw_data, const uint32_t stride, const size_t bytes_per_pixel) noexcept : raw_data_{static_cast<uint8_t *>(raw_data)},
                                                                                                                      bytes_per_pixel_{bytes_per_pixel},
                                                                                                                      bytes_per_line_{stride}
        {
        }

        void new_line_requested(void *destination, const size_t pixel_count, int /*destination_stride*/) noexcept(false) override
        {
            std::memcpy(destination, raw_data_, pixel_count * bytes_per_pixel_);
            raw_data_ += bytes_per_line_;
        }

        void new_line_decoded(const void *source, const size_t pixel_count, int /*sourceStride*/) noexcept(false) override
        {
            std::memcpy(raw_data_, source, pixel_count * bytes_per_pixel_);
            raw_data_ += bytes_per_line_;
        }

    private:
        uint8_t *raw_data_;
        size_t bytes_per_pixel_;
        size_t bytes_per_line_;
    };

    inline void byte_swap(void *data, const size_t count)
    {
        if (count & 1U)
            throw jpegls_error{jpegls_errc::invalid_encoded_data};

        auto *const data32 = static_cast<unsigned int *>(data);
        for (auto i = 0U; i < count / 4; ++i)
        {
            const auto value = data32[i];
            data32[i] = ((value >> 8U) & 0x00FF00FFU) | ((value & 0x00FF00FFU) << 8U);
        }

        auto *const data8 = static_cast<unsigned char *>(data);
        if ((count % 4) != 0)
        {
            std::swap(data8[count - 2], data8[count - 1]);
        }
    }

    class post_process_single_stream final : public process_line
    {
    public:
        post_process_single_stream(std::basic_streambuf<char> *raw_data, const uint32_t stride, const size_t bytes_per_pixel) noexcept : raw_data_{raw_data},
                                                                                                                                         bytes_per_pixel_{bytes_per_pixel},
                                                                                                                                         bytes_per_line_{stride}
        {
        }

        void new_line_requested(void *destination, const size_t pixel_count, int /*destination_stride*/) override
        {
            auto bytes_to_read = static_cast<std::streamsize>(static_cast<std::streamsize>(pixel_count) * bytes_per_pixel_);
            while (bytes_to_read != 0)
            {
                const auto bytes_read = raw_data_->sgetn(static_cast<char *>(destination), bytes_to_read);
                if (bytes_read == 0)
                    throw jpegls_error{jpegls_errc::destination_buffer_too_small};

                bytes_to_read = bytes_to_read - bytes_read;
            }

            if (bytes_per_pixel_ == 2)
            {
                byte_swap(static_cast<unsigned char *>(destination), 2U * pixel_count);
            }

            if (bytes_per_line_ - pixel_count * bytes_per_pixel_ > 0)
            {
                raw_data_->pubseekoff(static_cast<std::streamoff>(bytes_per_line_ - bytes_to_read), std::ios_base::cur);
            }
        }

        void new_line_decoded(const void *source, const size_t pixel_count, int /*source_stride*/) override
        {
            const auto bytes_to_write = pixel_count * bytes_per_pixel_;
            const auto bytes_written = static_cast<size_t>(raw_data_->sputn(static_cast<const char *>(source), static_cast<std::streamsize>(bytes_to_write)));
            if (bytes_written != bytes_to_write)
                throw jpegls_error{jpegls_errc::destination_buffer_too_small};
        }

    private:
        std::basic_streambuf<char> *raw_data_;
        size_t bytes_per_pixel_;
        size_t bytes_per_line_;
    };

    template <typename Transform, typename T>
    void transform_line_to_quad(const T *source, const size_t pixel_stride_in, quad<T> *destination, const size_t pixel_stride, Transform &transform) noexcept
    {
        const auto pixel_count = std::min(pixel_stride, pixel_stride_in);

        for (auto i = 0U; i < pixel_count; ++i)
        {
            const quad<T> pixel(transform(source[i], source[i + pixel_stride_in], source[i + 2 * pixel_stride_in]), source[i + 3 * pixel_stride_in]);
            destination[i] = pixel;
        }
    }

    template <typename Transform, typename T>
    void transform_quad_to_line(const quad<T> *source, const size_t pixel_stride_in, T *destination, const size_t pixel_stride, Transform &transform) noexcept
    {
        const auto pixel_count = std::min(pixel_stride, pixel_stride_in);

        for (auto i = 0U; i < pixel_count; ++i)
        {
            const quad<T> color = source[i];
            const quad<T> color_transformed(transform(color.v1, color.v2, color.v3), color.v4);

            destination[i] = color_transformed.v1;
            destination[i + pixel_stride] = color_transformed.v2;
            destination[i + 2 * pixel_stride] = color_transformed.v3;
            destination[i + 3 * pixel_stride] = color_transformed.v4;
        }
    }

    template <typename T>
    void transform_rgb_to_bgr(T *buffer, int samples_per_pixel, const size_t pixel_count) noexcept
    {
        for (auto i = 0U; i < pixel_count; ++i)
        {
            std::swap(buffer[0], buffer[2]);
            buffer += samples_per_pixel;
        }
    }

    template <typename Transform, typename T>
    void transform_line(triplet<T> *destination, const triplet<T> *source, const size_t pixel_count, Transform &transform) noexcept
    {
        for (auto i = 0U; i < pixel_count; ++i)
        {
            destination[i] = transform(source[i].v1, source[i].v2, source[i].v3);
        }
    }

    template <typename Transform, typename PixelType>
    void transform_line(quad<PixelType> *destination, const quad<PixelType> *source, const size_t pixel_count, Transform &transform) noexcept
    {
        for (auto i = 0U; i < pixel_count; ++i)
        {
            destination[i] = quad<PixelType>(transform(source[i].v1, source[i].v2, source[i].v3), source[i].v4);
        }
    }

    template <typename Transform, typename PixelType>
    void transform_line_to_triplet(const PixelType *source, const size_t pixel_stride_in, triplet<PixelType> *destination, const size_t pixel_stride, Transform &transform) noexcept
    {
        const auto pixel_count = std::min(pixel_stride, pixel_stride_in);
        triplet<PixelType> *type_buffer = destination;

        for (auto i = 0U; i < pixel_count; ++i)
        {
            type_buffer[i] = transform(source[i], source[i + pixel_stride_in], source[i + 2 * pixel_stride_in]);
        }
    }

    template <typename Transform, typename PixelType>
    void transform_triplet_to_line(const triplet<PixelType> *source, const size_t pixel_stride_in, PixelType *destination, const size_t pixel_stride, Transform &transform) noexcept
    {
        const auto pixel_count = std::min(pixel_stride, pixel_stride_in);
        const triplet<PixelType> *type_buffer_in = source;

        for (auto i = 0U; i < pixel_count; ++i)
        {
            const triplet<PixelType> color = type_buffer_in[i];
            const triplet<PixelType> color_transformed = transform(color.v1, color.v2, color.v3);

            destination[i] = color_transformed.v1;
            destination[i + pixel_stride] = color_transformed.v2;
            destination[i + 2 * pixel_stride] = color_transformed.v3;
        }
    }

    template <typename TransformType>
    class process_transformed final : public process_line
    {
    public:
        process_transformed(byte_stream_info raw_stream, const uint32_t stride, const frame_info &info, const coding_parameters &parameters, TransformType transform) : frame_info_{info},
                                                                                                                                                                        parameters_{parameters},
                                                                                                                                                                        stride_{stride},
                                                                                                                                                                        temp_line_(static_cast<size_t>(info.component_count) * info.width),
                                                                                                                                                                        buffer_(static_cast<size_t>(info.component_count) * info.width * sizeof(size_type)),
                                                                                                                                                                        transform_{transform},
                                                                                                                                                                        inverse_transform_{transform},
                                                                                                                                                                        raw_pixels_{raw_stream}
        {
        }

        void new_line_requested(void *destination, const size_t pixel_count, const int destination_stride) override
        {
            if (!raw_pixels_.rawStream)
            {
                transform(raw_pixels_.rawData, destination, pixel_count, destination_stride);
                raw_pixels_.rawData += stride_;
                return;
            }

            transform(raw_pixels_.rawStream, destination, pixel_count, destination_stride);
        }

        void transform(std::basic_streambuf<char> *raw_stream, void *destination, const size_t pixel_count, const int destination_stride)
        {
            std::streamsize bytes_to_read = static_cast<std::streamsize>(pixel_count) * frame_info_.component_count * sizeof(size_type);
            while (bytes_to_read != 0)
            {
                const auto read = raw_stream->sgetn(reinterpret_cast<char *>(buffer_.data()), bytes_to_read);
                if (read == 0)
                    throw jpegls_error{jpegls_errc::source_buffer_too_small};

                bytes_to_read -= read;
            }
            transform(buffer_.data(), destination, pixel_count, destination_stride);
        }

        void transform(const void *source, void *destination, const size_t pixel_count, const size_t destination_stride) noexcept
        {
            if (parameters_.output_bgr)
            {
                memcpy(temp_line_.data(), source, sizeof(triplet<size_type>) * pixel_count);
                transform_rgb_to_bgr(temp_line_.data(), frame_info_.component_count, pixel_count);
                source = temp_line_.data();
            }

            if (frame_info_.component_count == 3)
            {
                if (parameters_.interleave_mode == interleave_mode::sample)
                {
                    transform_line(static_cast<triplet<size_type> *>(destination), static_cast<const triplet<size_type> *>(source), pixel_count, transform_);
                }
                else
                {
                    transform_triplet_to_line(static_cast<const triplet<size_type> *>(source), pixel_count, static_cast<size_type *>(destination), destination_stride, transform_);
                }
            }
            else if (frame_info_.component_count == 4)
            {
                if (parameters_.interleave_mode == interleave_mode::sample)
                {
                    transform_line(static_cast<quad<size_type> *>(destination), static_cast<const quad<size_type> *>(source), pixel_count, transform_);
                }
                else if (parameters_.interleave_mode == interleave_mode::line)
                {
                    transform_quad_to_line(static_cast<const quad<size_type> *>(source), pixel_count, static_cast<size_type *>(destination), destination_stride, transform_);
                }
            }
        }

        void decode_transform(const void *source, void *destination, const size_t pixel_count, const size_t byte_stride) noexcept
        {
            if (frame_info_.component_count == 3)
            {
                if (parameters_.interleave_mode == interleave_mode::sample)
                {
                    transform_line(static_cast<triplet<size_type> *>(destination), static_cast<const triplet<size_type> *>(source), pixel_count, inverse_transform_);
                }
                else
                {
                    transform_line_to_triplet(static_cast<const size_type *>(source), byte_stride, static_cast<triplet<size_type> *>(destination), pixel_count, inverse_transform_);
                }
            }
            else if (frame_info_.component_count == 4)
            {
                if (parameters_.interleave_mode == interleave_mode::sample)
                {
                    transform_line(static_cast<quad<size_type> *>(destination), static_cast<const quad<size_type> *>(source), pixel_count, inverse_transform_);
                }
                else if (parameters_.interleave_mode == interleave_mode::line)
                {
                    transform_line_to_quad(static_cast<const size_type *>(source), byte_stride, static_cast<quad<size_type> *>(destination), pixel_count, inverse_transform_);
                }
            }

            if (parameters_.output_bgr)
            {
                transform_rgb_to_bgr(static_cast<size_type *>(destination), frame_info_.component_count, pixel_count);
            }
        }

        void new_line_decoded(const void *source, const size_t pixel_count, const int source_stride) override
        {
            if (raw_pixels_.rawStream)
            {
                const std::streamsize bytes_to_write = static_cast<std::streamsize>(pixel_count) * frame_info_.component_count * sizeof(size_type);
                decode_transform(source, buffer_.data(), pixel_count, source_stride);

                const auto bytes_written = raw_pixels_.rawStream->sputn(reinterpret_cast<char *>(buffer_.data()), bytes_to_write);
                if (bytes_written != bytes_to_write)
                    throw jpegls_error{jpegls_errc::destination_buffer_too_small};
            }
            else
            {
                decode_transform(source, raw_pixels_.rawData, pixel_count, source_stride);
                raw_pixels_.rawData += stride_;
            }
        }

    private:
        using size_type = typename TransformType::size_type;

        const frame_info &frame_info_;
        const coding_parameters &parameters_;
        const uint32_t stride_;
        std::vector<size_type> temp_line_;
        std::vector<uint8_t> buffer_;
        TransformType transform_;
        typename TransformType::inverse inverse_transform_;
        byte_stream_info raw_pixels_;
    };

} // namespace charls
