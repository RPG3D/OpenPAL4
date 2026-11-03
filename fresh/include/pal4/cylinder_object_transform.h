#pragma once

#include "pal4/cylinder_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ICylinderObjectTransformRuntime {
public:
    virtual ~ICylinderObjectTransformRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t SetPosition(
        std::uint32_t model, const std::array<float, 3>& value,
        std::uint8_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t SetRotation(
        std::uint32_t model, const std::array<float, 3>& value,
        std::uint8_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t SetScale(
        std::uint32_t model, const std::array<float, 3>& value,
        std::uint8_t mode) = 0;
};

class CylinderObjectTransformRuntimeStub final
    : public ICylinderObjectTransformRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t SetPosition(
        std::uint32_t, const std::array<float, 3>&,
        std::uint8_t) override { return 0u; }
    [[nodiscard]] std::uint8_t SetRotation(
        std::uint32_t, const std::array<float, 3>&,
        std::uint8_t) override { return 0u; }
    [[nodiscard]] std::uint8_t SetScale(
        std::uint32_t, const std::array<float, 3>&,
        std::uint8_t) override { return 0u; }
};

class CylinderObjectTransform final {
public:
    // IDA 0x60d540/0x60d570/0x60d5a0.
    [[nodiscard]] static std::uint8_t GetRotation(
        const CylinderObjectState& state,
        std::array<std::uint32_t, 3>& output,
        ICylinderObjectTransformRuntime& runtime);
    [[nodiscard]] static std::uint8_t GetScale(
        const CylinderObjectState& state,
        std::array<std::uint32_t, 3>& output,
        ICylinderObjectTransformRuntime& runtime);
    [[nodiscard]] static std::uint8_t GetPosition(
        const CylinderObjectState& state,
        std::array<std::uint32_t, 3>& output,
        ICylinderObjectTransformRuntime& runtime);
    // IDA 0x60d5d0/0x60d610/0x60d650.
    [[nodiscard]] static std::uint8_t SetScale(
        const CylinderObjectState& state,
        const std::array<float, 3>& value, std::uint8_t mode,
        ICylinderObjectTransformRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetRotation(
        const CylinderObjectState& state,
        const std::array<float, 3>& value, std::uint8_t mode,
        ICylinderObjectTransformRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetPosition(
        const CylinderObjectState& state,
        const std::array<float, 3>& value, std::uint8_t mode,
        ICylinderObjectTransformRuntime& runtime);
};

}  // namespace pal4
