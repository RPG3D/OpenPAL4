#include "pal4/thunder_visual_path.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace pal4 {
namespace {

float ReadFloat(const ThunderObjectState& state, std::uint32_t offset) {
    return std::bit_cast<float>(state.words[(offset + 60u) / 4u]);
}

std::uint8_t ReadByte(const ThunderObjectState& state,
                      std::uint32_t offset) {
    offset += 60u;
    return static_cast<std::uint8_t>(state.words[offset / 4u] >>
                                     (8u * (offset % 4u)));
}

double RandomRangeDouble(IThunderVisualPathRuntime& runtime,
                         float minimum, float maximum) {
    constexpr double scale =
        static_cast<double>(std::bit_cast<float>(0x38000100u));
    return static_cast<double>(runtime.Random()) *
               (static_cast<double>(maximum) - minimum) * scale + minimum;
}

float RandomRange(IThunderVisualPathRuntime& runtime,
                  float minimum, float maximum) {
    return static_cast<float>(
        RandomRangeDouble(runtime, minimum, maximum));
}

}  // namespace

ThunderVisualSegment* ThunderSegmentRecords::Copy(
    const ThunderVisualSegment* source_begin,
    const ThunderVisualSegment* source_end,
    ThunderVisualSegment* destination) {
    static_assert(sizeof(ThunderVisualSegment) == 24u);
    while (source_begin != source_end) {
        std::memcpy(destination, source_begin, sizeof(ThunderVisualSegment));
        ++source_begin;
        ++destination;
    }
    return destination;
}

std::uint8_t ThunderVisualPath::Build(
    ThunderObjectState& state, IThunderVisualPathRuntime& runtime) {
    const float length = ReadFloat(state, 60u);
    const float half_length = static_cast<float>(
        static_cast<double>(length) * 0.5);
    const float min_range = ReadFloat(state, 100u) * length;
    const float max_range = ReadFloat(state, 104u) * length;
    const float min_width = ReadFloat(state, 108u) * length;
    const float max_width = ReadFloat(state, 112u) * length;
    const float min_section = ReadFloat(state, 116u) * length;
    const float max_section = ReadFloat(state, 120u) * length;
    const bool fixed_width = ReadByte(state, 4u) != 0u;
    const bool fixed_range = ReadByte(state, 5u) != 0u;
    const bool lock_end = ReadByte(state, 3u) != 0u;

    std::vector<ThunderVisualSegment> segments;
    runtime.WriteSegments(state, segments);
    if (length > 0.0f) {
        float remaining = length;
        float y = -half_length;
        bool first = true;
        constexpr double width_scale =
            static_cast<double>(std::bit_cast<float>(0x37800100u));
        constexpr double epsilon =
            static_cast<double>(std::bit_cast<float>(0x38d1b717u));
        do {
            float width_half;
            if (fixed_width) {
                width_half = static_cast<float>(max_width * 0.5);
            } else {
                width_half = static_cast<float>(
                    static_cast<double>(runtime.Random()) *
                        (static_cast<double>(max_width) - min_width) *
                        width_scale + static_cast<double>(min_width) * 0.5);
            }
            const float range = fixed_range
                                    ? min_range
                                    : RandomRange(runtime, min_range, max_range);
            if (first) {
                segments.push_back({{-width_half, y, 0.0f},
                                    {width_half, y, 0.0f}});
                first = false;
            } else {
                const double step = remaining >= max_section
                                        ? RandomRangeDouble(runtime, min_section,
                                                            max_section)
                                        : static_cast<double>(remaining);
                if (!(step > 0.0f)) break;
                const float next_remaining = static_cast<float>(
                    static_cast<double>(remaining) - step);
                if (next_remaining == remaining) break;
                remaining = next_remaining;
                y = static_cast<float>(static_cast<double>(y) + step);
                const bool snap_end = lock_end &&
                    static_cast<double>(y) + epsilon > half_length &&
                    static_cast<double>(y) - epsilon < half_length;
                if (snap_end) {
                    segments.push_back({{-width_half, half_length, 0.0f},
                                        {width_half, half_length, 0.0f}});
                } else {
                    const float x = RandomRange(runtime, -range, range);
                    const float z = RandomRange(runtime, -range, range);
                    segments.push_back({{x - width_half, y, z},
                                        {x + width_half, y, z}});
                }
            }
        } while (remaining > 0.0f);
    }

    const auto count = segments.size();
    if (fixed_width && count > 1u) {
        const float half_step = static_cast<float>(
            (static_cast<double>(max_width) - min_width) /
            static_cast<double>(count - 1u) * 0.5);
        for (std::size_t i = 0u; i < count; ++i) {
            const float shift = static_cast<float>(
                static_cast<double>(i) * half_step);
            segments[i].first[0] = static_cast<float>(
                static_cast<double>(segments[i].first[0]) + shift);
            segments[i].second[0] = static_cast<float>(
                static_cast<double>(segments[i].second[0]) - shift);
        }
    }
    if (fixed_range && count > 1u) {
        const float step = static_cast<float>(
            (static_cast<double>(max_range) - min_range) /
            static_cast<double>(count - 1u));
        const auto limit = lock_end ? count - 1u : count;
        for (std::size_t i = 1u; i < limit; ++i) {
            const float x = static_cast<float>(
                RandomRangeDouble(runtime, -step, step) * i);
            const float z = static_cast<float>(
                RandomRangeDouble(runtime, -step, step) * i);
            for (auto* point : {&segments[i].first, &segments[i].second}) {
                (*point)[0] = static_cast<float>(
                    static_cast<double>((*point)[0]) + x);
                (*point)[2] = static_cast<float>(
                    static_cast<double>((*point)[2]) + z);
            }
        }
    }
    runtime.WriteSegments(state, segments);

    const auto sample = ReadByte(state, 6u);
    const auto coefficient = static_cast<std::int32_t>(state.words[17u]);
    if (sample == 0u || coefficient <= 0 || count < 4u)
        return sample;

    std::vector<ThunderVector3> first_points;
    std::vector<ThunderVector3> second_points;
    first_points.reserve(count - 1u);
    second_points.reserve(count - 1u);
    for (std::size_t i = 1u; i < count; ++i) {
        first_points.push_back(segments[i].first);
        second_points.push_back(segments[i].second);
    }
    const auto first_spline = runtime.CreateSpline(first_points, 1u);
    const auto second_spline = runtime.CreateSpline(second_points, 1u);
    if (first_spline == 0u || second_spline == 0u) {
        if (first_spline != 0u) (void)runtime.DestroySpline(first_spline);
        if (second_spline != 0u) (void)runtime.DestroySpline(second_spline);
        return 0u;
    }

    const auto sample_count = static_cast<std::size_t>(coefficient) * count;
    std::vector<ThunderVisualSegment> sampled;
    sampled.reserve(sample_count);
    const float parameter_step = static_cast<float>(
        1.0 / static_cast<double>(sample_count - 1u));
    for (std::size_t i = 0u; i < sample_count; ++i) {
        const float t = static_cast<float>(
            static_cast<double>(i) * parameter_step);
        sampled.push_back({runtime.SplinePosition(first_spline, 11u, t),
                           runtime.SplinePosition(second_spline, 11u, t)});
    }
    runtime.WriteSegments(state, sampled);
    (void)runtime.DestroySpline(first_spline);
    return runtime.DestroySpline(second_spline);
}

}  // namespace pal4
