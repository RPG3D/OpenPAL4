#pragma once

#include "pal4/effect_system_object_f.h"

#include <cstdint>
#include <stdexcept>

namespace pal4 {

class IEffectSystemObjectFValidationRuntime {
public:
    virtual ~IEffectSystemObjectFValidationRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t EffectSystemValidator() = 0;
    [[nodiscard]] virtual std::uint8_t ValidateTransform(
        std::uint32_t validator, std::uint32_t source,
        std::uint32_t transform_area) = 0;
    [[nodiscard]] virtual std::uint8_t CheckObject(
        std::uint32_t object, std::uint32_t register_ebp,
        std::uint32_t register_edi) = 0;
    [[noreturn]] virtual void ThrowMissingValidator() = 0;
};

class EffectSystemObjectFValidationRuntimeStub final
    : public IEffectSystemObjectFValidationRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetObjectTransform(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t EffectSystemValidator() override { return 0u; }
    [[nodiscard]] std::uint8_t ValidateTransform(
        std::uint32_t, std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t CheckObject(
        std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    [[noreturn]] void ThrowMissingValidator() override {
        throw std::runtime_error("EffectSystem validator is unavailable");
    }
};

class EffectSystemObjectFValidation final {
public:
    // IDA 0x605f30. EBP and EDI are forwarded unchanged by the native helper.
    [[nodiscard]] static std::uint8_t Check(
        const EffectSystemObjectFState& state,
        std::uint32_t register_ebp, std::uint32_t register_edi,
        std::uint32_t source, IEffectSystemObjectFValidationRuntime& runtime);
};

}  // namespace pal4
