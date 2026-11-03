#pragma once

#include "pal4/clump_object.h"
#include "pal4/clump_object_transform.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ClumpObjectAimFacet final {
public:
    // IDA 0x60b4b0, object +52 facet.
    [[nodiscard]] static std::uint8_t SetPosition(
        const ClumpObjectState& state, const std::array<float, 3>& position,
        IClumpObjectTransformRuntime& runtime);
    // IDA 0x60b4f0, object +56 facet.
    [[nodiscard]] static std::uint8_t GetPosition(
        const ClumpObjectState& state, std::array<std::uint32_t, 3>& output,
        IClumpObjectTransformRuntime& runtime);
    // IDA 0x60b520/0x60b560, object +56 facet.
    [[nodiscard]] static std::uint8_t SetTarget(
        ClumpObjectState& state, const std::array<std::uint32_t, 3>& target);
    [[nodiscard]] static std::uint8_t SetRotationOffset(
        ClumpObjectState& state, const std::array<std::uint32_t, 3>& offset);
    // IDA 0x60b580.
    [[nodiscard]] static std::uint8_t UpdateFrame(
        ClumpObjectState& state, std::uint8_t enabled,
        class IClumpObjectAimRuntime& runtime);
    // IDA 0x60b820, object +56 facet.
    [[nodiscard]] static std::uint8_t SetPositionWithMode(
        const ClumpObjectState& state, const std::array<float, 3>& position,
        std::uint8_t mode, IClumpObjectTransformRuntime& runtime);
};

struct ClumpAimMatrix final {
    std::array<float, 3> right{};
    std::uint32_t flags = 0x20003u;
    std::array<float, 3> up{};
    std::uint32_t pad1 = 0u;
    std::array<float, 3> at{};
    std::uint32_t pad2 = 0u;
    std::array<float, 3> position{};
    std::uint32_t pad3 = 0u;
};

static_assert(sizeof(ClumpAimMatrix) == 64u);

class IClumpObjectAimRuntime {
public:
    virtual ~IClumpObjectAimRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void Normalize(std::array<float, 3>& vector) = 0;
    virtual void UpdateMatrix(ClumpAimMatrix& matrix) = 0;
    [[nodiscard]] virtual std::uint32_t GetFrame(
        const ClumpObjectState& state) = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void ScaleFrame(std::uint32_t frame,
                            const std::array<std::uint32_t, 3>& scale,
                            std::uint32_t mode) = 0;
    virtual void ApplyMatrix(std::uint32_t frame,
                             const ClumpAimMatrix& matrix,
                             std::uint32_t mode) = 0;
    virtual void TranslateFrame(std::uint32_t frame,
                                const std::array<std::uint32_t, 3>& position,
                                std::uint32_t mode) = 0;
    virtual void UpdateFrameRotation(
        std::uint32_t frame,
        const std::array<std::uint32_t, 3>& offset,
        std::array<float, 3>& output) = 0;
    [[nodiscard]] virtual std::uint8_t SetRotation(
        ClumpObjectState& state, const std::array<float, 3>& angles,
        std::uint32_t mode) = 0;
};

class ClumpObjectAimRuntimeStub final : public IClumpObjectAimRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void Normalize(std::array<float, 3>&) override {}
    void UpdateMatrix(ClumpAimMatrix&) override {}
    [[nodiscard]] std::uint32_t GetFrame(const ClumpObjectState&) override {
        return 0u;
    }
    void SetFrameIdentity(std::uint32_t) override {}
    void ScaleFrame(std::uint32_t,
                    const std::array<std::uint32_t, 3>&,
                    std::uint32_t) override {}
    void ApplyMatrix(std::uint32_t, const ClumpAimMatrix&,
                     std::uint32_t) override {}
    void TranslateFrame(std::uint32_t,
                        const std::array<std::uint32_t, 3>&,
                        std::uint32_t) override {}
    void UpdateFrameRotation(std::uint32_t,
                             const std::array<std::uint32_t, 3>&,
                             std::array<float, 3>&) override {}
    [[nodiscard]] std::uint8_t SetRotation(
        ClumpObjectState&, const std::array<float, 3>&,
        std::uint32_t) override { return 0u; }
};

}  // namespace pal4
