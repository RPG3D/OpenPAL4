#pragma once

#include "pal4/cylinder_object.h"

#include <cstdint>

namespace pal4 {

class ICylinderObjectRenderRuntime {
public:
    virtual ~ICylinderObjectRenderRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t EffectManager() = 0;
    [[nodiscard]] virtual std::uint8_t ValidateTransform(
        std::uint32_t manager, std::uint32_t context,
        std::uint32_t transform) = 0;
    [[nodiscard]] virtual std::uint8_t RenderModel(
        std::uint32_t model, std::uint32_t ebp_value,
        std::uint32_t context, std::uint32_t repeated_context) = 0;
};

class CylinderObjectRenderRuntimeStub final
    : public ICylinderObjectRenderRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetObjectTransform(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t EffectManager() override { return 0u; }
    [[nodiscard]] std::uint8_t ValidateTransform(
        std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t RenderModel(
        std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class CylinderObjectRenderer final {
public:
    // IDA 0x60d470.
    [[nodiscard]] static std::uint8_t Render(
        const CylinderObjectState& state, std::uint32_t ebp_value,
        std::uint32_t context, ICylinderObjectRenderRuntime& runtime);
};

}  // namespace pal4
