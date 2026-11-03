#pragma once

#include "pal4/strip_object.h"

#include <array>
#include <cstdint>
#include <span>

namespace pal4 {

enum class StripTransformKind { Scale, Rotation, Position };

class IStripPropertyRuntime {
public:
    virtual ~IStripPropertyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UpdateStraight(
        std::uint32_t model, StripTransformKind kind,
        const std::array<float, 3>& values) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateFrame(
        StripObjectState& state) = 0;
};

class StripProperties final {
public:
    // IDA 0x619ce0, 0x619d10, 0x619d40.
    [[nodiscard]] static std::uint8_t GetRotation(
        const StripObjectState& state, std::array<std::uint32_t, 3>& output);
    // Shared619CE0 body also serves EffectObject's position facet+28.
    // Both owners expose active byte+4 and the triple at+76 in this prefix.
    [[nodiscard]] static std::uint8_t GetRotation(
        std::span<const std::uint32_t,22u> common_prefix,
        std::array<std::uint32_t,3>& output);
    [[nodiscard]] static std::uint8_t GetScale(
        const StripObjectState& state, std::array<std::uint32_t, 3>& output);
    // Shared619D10 also serves EffectObject rotation at owner+24.
    [[nodiscard]] static std::uint8_t GetScale(
        std::span<const std::uint32_t,19u> common_prefix,
        std::array<std::uint32_t,3>& output);
    [[nodiscard]] static std::uint8_t GetPosition(
        const StripObjectState& state, std::array<std::uint32_t, 3>& output);

    // IDA 0x619d70, 0x619de0, 0x619e50.
    [[nodiscard]] static std::uint32_t SetScale(
        StripObjectState& state, const std::array<float, 3>& values,
        bool relative, IStripPropertyRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetRotation(
        StripObjectState& state, const std::array<float, 3>& values,
        bool relative, IStripPropertyRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetPosition(
        StripObjectState& state, const std::array<float, 3>& values,
        bool relative, IStripPropertyRuntime& runtime);
};

}  // namespace pal4
