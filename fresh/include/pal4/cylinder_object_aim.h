#pragma once

#include "pal4/cylinder_object.h"
#include "pal4/cylinder_object_transform.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ICylinderObjectAimRuntime {
public:
    virtual ~ICylinderObjectAimRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class CylinderObjectAimRuntimeStub final : public ICylinderObjectAimRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
};

class CylinderObjectAim final {
public:
    // IDA 0x60dbe0, object +52 facet.
    [[nodiscard]] static std::uint8_t GetPosition(
        const CylinderObjectState& state,
        std::array<std::uint32_t, 3>& output,
        ICylinderObjectAimRuntime& runtime);
    // IDA 0x60dc10, raw rotation-offset words at object +72/+76/+80.
    [[nodiscard]] static std::uint8_t SetRotationOffset(
        CylinderObjectState& state,
        const std::array<std::uint32_t, 3>& offset);
    // IDA 0x60dc30.
    [[nodiscard]] static std::uint8_t UpdateFrame(
        CylinderObjectState& state, std::uint8_t enabled,
        class ICylinderObjectAimFrameRuntime& runtime);
    // IDA 0x60ded0, same object +52 facet.
    [[nodiscard]] static std::uint8_t SetPositionWithMode(
        const CylinderObjectState& state,
        const std::array<float, 3>& position,
        std::uint8_t mode,
        ICylinderObjectTransformRuntime& runtime);
};

struct CylinderAimMatrix final {
    std::array<float, 3> right{};
    std::uint32_t flags = 0x20003u;
    std::array<float, 3> up{};
    std::uint32_t pad1 = 0u;
    std::array<float, 3> at{};
    std::uint32_t pad2 = 0u;
    std::array<float, 3> position{};
    std::uint32_t pad3 = 0u;
};

static_assert(sizeof(CylinderAimMatrix) == 64u);

class ICylinderObjectAimFrameRuntime {
public:
    virtual ~ICylinderObjectAimFrameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void Normalize(std::array<float, 3>& vector) = 0;
    virtual void UpdateMatrix(CylinderAimMatrix& matrix) = 0;
    [[nodiscard]] virtual std::uint32_t GetFrame(
        const CylinderObjectState& state) = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void ScaleFrame(std::uint32_t frame,
                            const std::array<std::uint32_t, 3>& scale,
                            std::uint32_t mode) = 0;
    virtual void ApplyMatrix(std::uint32_t frame,
                             const CylinderAimMatrix& matrix,
                             std::uint32_t mode) = 0;
    virtual void TranslateFrame(std::uint32_t frame,
                                const std::array<std::uint32_t, 3>& position,
                                std::uint32_t mode) = 0;
    virtual void UpdateFrameRotation(
        std::uint32_t frame,
        const std::array<std::uint32_t, 3>& offset,
        std::array<float, 3>& output) = 0;
    [[nodiscard]] virtual std::uint8_t SetRotation(
        CylinderObjectState& state, const std::array<float, 3>& angles,
        std::uint32_t mode) = 0;
};

class CylinderObjectAimFrameRuntimeStub final
    : public ICylinderObjectAimFrameRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void Normalize(std::array<float, 3>&) override {}
    void UpdateMatrix(CylinderAimMatrix&) override {}
    [[nodiscard]] std::uint32_t GetFrame(const CylinderObjectState&) override {
        return 0u;
    }
    void SetFrameIdentity(std::uint32_t) override {}
    void ScaleFrame(std::uint32_t,
                    const std::array<std::uint32_t, 3>&,
                    std::uint32_t) override {}
    void ApplyMatrix(std::uint32_t, const CylinderAimMatrix&,
                     std::uint32_t) override {}
    void TranslateFrame(std::uint32_t,
                        const std::array<std::uint32_t, 3>&,
                        std::uint32_t) override {}
    void UpdateFrameRotation(std::uint32_t,
                             const std::array<std::uint32_t, 3>&,
                             std::array<float, 3>&) override {}
    [[nodiscard]] std::uint8_t SetRotation(
        CylinderObjectState&, const std::array<float, 3>&,
        std::uint32_t) override { return 0u; }
};

}  // namespace pal4
