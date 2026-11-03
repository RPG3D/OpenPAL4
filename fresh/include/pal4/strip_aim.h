#pragma once

#include "pal4/strip_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct StripAimMatrix final {
    std::array<float, 3> right{};
    std::uint32_t flags = 0x20003u;
    std::array<float, 3> up{};
    std::uint32_t pad1 = 0u;
    std::array<float, 3> at{};
    std::uint32_t pad2 = 0u;
    std::array<float, 3> position{};
    std::uint32_t pad3 = 0u;
};
static_assert(sizeof(StripAimMatrix) == 64u);

class IStripAimRuntime {
public:
    virtual ~IStripAimRuntime() = default;
    virtual void ReadStraightPosition(
        std::uint32_t model, std::array<float, 3>& output) = 0;
    virtual void Normalize(std::array<float, 3>& vector) = 0;
    virtual void UpdateMatrix(StripAimMatrix& matrix) = 0;
    [[nodiscard]] virtual std::uint32_t GetFrame(
        StripObjectState& state) = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void ScaleFrame(std::uint32_t frame,
                            const std::array<std::uint32_t, 3>& scale,
                            std::uint32_t mode) = 0;
    virtual void ApplyMatrix(std::uint32_t frame,
                             const StripAimMatrix& matrix,
                             std::uint32_t mode) = 0;
    virtual void TranslateFrame(
        std::uint32_t frame,
        const std::array<std::uint32_t, 3>& position,
        std::uint32_t mode) = 0;
    virtual void UpdateFrameRotation(
        std::uint32_t frame,
        const std::array<std::uint32_t, 3>& offset,
        std::array<float, 3>& output) = 0;
    [[nodiscard]] virtual std::uint32_t SetRotation(
        StripObjectState& state, const std::array<float, 3>& angles,
        std::uint32_t mode) = 0;
};

class IStripPropertyRuntime;

class StripAim final {
public:
    // IDA 0x61a490/0x61a4c0/0x61a500.
    [[nodiscard]] static std::uint8_t GetPosition(
        const StripObjectState& state,
        std::array<std::uint32_t, 3>& output);
    [[nodiscard]] static std::uint8_t SetTarget(
        StripObjectState& state,
        const std::array<std::uint32_t, 3>& values);
    [[nodiscard]] static std::uint8_t SetRotationOffset(
        StripObjectState& state,
        const std::array<std::uint32_t, 3>& values);
    // IDA 0x61a7e0 forwards position writes through the +52 facet.
    [[nodiscard]] static std::uint32_t SetPosition(
        StripObjectState& state, const std::array<float, 3>& values,
        bool relative, IStripPropertyRuntime& runtime);
    // IDA 0x61a520.
    [[nodiscard]] static std::uint32_t UpdateFrame(
        StripObjectState& state, std::uint8_t enabled,
        IStripAimRuntime& runtime);
};

}  // namespace pal4
