// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "decoder_strategy.h"
#include "default_traits.h"
#include "encoder_strategy.h"
#include "jls_codec_factory.h"
#include "jpeg_stream_reader.h"
#include "jpegls_preset_coding_parameters.h"
#include "lookup_table.h"
#include "lossless_traits.h"
#include "scan.h"
#include "util.h"

#include <array>
#include <vector>

using std::array;
using std::make_unique;
using std::unique_ptr;
using std::vector;
using namespace charls;

namespace {

// See JPEG-LS standard ISO/IEC 14495-1, A.3.3, golomb_code Segment A.4
int8_t quantize_gradient_org(const jpegls_pc_parameters& preset, const int32_t di) noexcept
{
    constexpr int32_t near_lossless = 0;

    if (di <= -preset.threshold3) return -4;
    if (di <= -preset.threshold2) return -3;
    if (di <= -preset.threshold1) return -2;
    if (di < -near_lossless) return -1;
    if (di <= near_lossless) return 0;
    if (di < preset.threshold1) return 1;
    if (di < preset.threshold2) return 2;
    if (di < preset.threshold3) return 3;

    return 4;
}

vector<int8_t> create_quantize_lut_lossless(const int32_t bit_count)
{
    const jpegls_pc_parameters preset{compute_default((1 << static_cast<uint32_t>(bit_count)) - 1, 0)};
    const int32_t range = preset.maximum_sample_value + 1;

    vector<int8_t> lut(static_cast<size_t>(range) * 2);
    for (size_t i = 0; i < lut.size(); ++i)
    {
        lut[i] = quantize_gradient_org(preset, static_cast<int32_t>(i) - range);
    }

    return lut;
}

template<typename Strategy, typename Traits>
unique_ptr<Strategy> make_codec(const Traits& traits, const frame_info& frame_info, const coding_parameters& parameters)
{
    return make_unique<charls::jls_codec<Traits, Strategy>>(traits, frame_info, parameters);
}

} // namespace


namespace charls {

// Lookup tables to replace code with lookup tables.
// To avoid threading issues, all tables are created when the program is loaded.

// Lookup table: decode symbols that are smaller or equal to 8 bit (16 tables for each value of k)
array<golomb_code_table, 16> decoding_tables = {initialize_table(0), initialize_table(1), initialize_table(2), initialize_table(3), // NOLINT(clang-diagnostic-global-constructors)
                                    initialize_table(4), initialize_table(5), initialize_table(6), initialize_table(7),
                                    initialize_table(8), initialize_table(9), initialize_table(10), initialize_table(11),
                                    initialize_table(12), initialize_table(13), initialize_table(14), initialize_table(15)};

// Lookup tables: sample differences to bin indexes.
vector<int8_t> quantization_lut_lossless_8 = create_quantize_lut_lossless(8);   // NOLINT(clang-diagnostic-global-constructors)
vector<int8_t> quantization_lut_lossless_10 = create_quantize_lut_lossless(10); // NOLINT(clang-diagnostic-global-constructors)
vector<int8_t> quantization_lut_lossless_12 = create_quantize_lut_lossless(12); // NOLINT(clang-diagnostic-global-constructors)
vector<int8_t> quantization_lut_lossless_16 = create_quantize_lut_lossless(16); // NOLINT(clang-diagnostic-global-constructors)


template<typename Strategy>
unique_ptr<Strategy> jls_codec_factory<Strategy>::create_codec(const frame_info& frame, const coding_parameters& parameters, const jpegls_pc_parameters& preset_coding_parameters)
{
    unique_ptr<Strategy> codec;

    if (preset_coding_parameters.reset_value == 0 || preset_coding_parameters.reset_value == default_reset_value)
    {
        codec = create_optimized_codec(frame, parameters);
    }

    if (!codec)
    {
        if (frame.bits_per_sample <= 8)
        {
            default_traits<uint8_t, uint8_t> traits(static_cast<int32_t>(calculate_maximum_sample_value(frame.bits_per_sample)), parameters.near_lossless, preset_coding_parameters.reset_value);
            traits.maximum_sample_value = preset_coding_parameters.maximum_sample_value;
            codec = make_unique<jls_codec<default_traits<uint8_t, uint8_t>, Strategy>>(traits, frame, parameters);
        }
        else
        {
            default_traits<uint16_t, uint16_t> traits(static_cast<int32_t>(calculate_maximum_sample_value(frame.bits_per_sample)), parameters.near_lossless, preset_coding_parameters.reset_value);
            traits.maximum_sample_value = preset_coding_parameters.maximum_sample_value;
            codec = make_unique<jls_codec<default_traits<uint16_t, uint16_t>, Strategy>>(traits, frame, parameters);
        }
    }

    codec->set_presets(preset_coding_parameters);
    return codec;
}

template<typename Strategy>
unique_ptr<Strategy> jls_codec_factory<Strategy>::create_optimized_codec(const frame_info& frame, const coding_parameters& parameters)
{
    if (parameters.interleave_mode == interleave_mode::sample && frame.component_count != 3 && frame.component_count != 4)
        return nullptr;

#ifndef DISABLE_SPECIALIZATIONS

    // optimized lossless versions common formats
    if (parameters.near_lossless == 0)
    {
        if (parameters.interleave_mode == interleave_mode::sample)
        {
            if (frame.component_count == 3 && frame.bits_per_sample == 8)
                return make_codec<Strategy>(lossless_traits<triplet<uint8_t>, 8>(), frame, parameters);
            if (frame.component_count == 4 && frame.bits_per_sample == 8)
                return make_codec<Strategy>(lossless_traits<quad<uint8_t>, 8>(), frame, parameters);
        }
        else
        {
            switch (frame.bits_per_sample)
            {
            case 8:
                return make_codec<Strategy>(lossless_traits<uint8_t, 8>(), frame, parameters);
            case 12:
                return make_codec<Strategy>(lossless_traits<uint16_t, 12>(), frame, parameters);
            case 16:
                return make_codec<Strategy>(lossless_traits<uint16_t, 16>(), frame, parameters);
            default:
                break;
            }
        }
    }

#endif

    const auto maxval = static_cast<int>(calculate_maximum_sample_value(frame.bits_per_sample));

    if (frame.bits_per_sample <= 8)
    {
        if (parameters.interleave_mode == interleave_mode::sample)
        {
            if (frame.component_count == 3)
                return make_codec<Strategy>(default_traits<uint8_t, triplet<uint8_t>>(maxval, parameters.near_lossless), frame, parameters);
            if (frame.component_count == 4)
                return make_codec<Strategy>(default_traits<uint8_t, quad<uint8_t>>(maxval, parameters.near_lossless), frame, parameters);
        }

        return make_codec<Strategy>(default_traits<uint8_t, uint8_t>(maxval, parameters.near_lossless), frame, parameters);
    }
    if (frame.bits_per_sample <= 16)
    {
        if (parameters.interleave_mode == interleave_mode::sample)
        {
            if (frame.component_count == 3)
                return make_codec<Strategy>(default_traits<uint16_t, triplet<uint16_t>>(maxval, parameters.near_lossless), frame, parameters);
            if (frame.component_count == 4)
                return make_codec<Strategy>(default_traits<uint16_t, quad<uint16_t>>(maxval, parameters.near_lossless), frame, parameters);
        }

        return make_codec<Strategy>(default_traits<uint16_t, uint16_t>(maxval, parameters.near_lossless), frame, parameters);
    }
    return nullptr;
}


template class jls_codec_factory<decoder_strategy>;
template class jls_codec_factory<encoder_strategy>;

} // namespace charls
