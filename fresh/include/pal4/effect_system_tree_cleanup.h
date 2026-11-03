#pragma once

#include "pal4/effect_system_registration.h"

#include <cstdint>

namespace pal4 {

class IEffectSystemTreeCleanupRuntime {
public:
    virtual ~IEffectSystemTreeCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Nil(
        EffectRegistrationTree kind) = 0;
    virtual void DeleteSubtree(EffectRegistrationTree kind,
                               std::uint32_t root) = 0;
    virtual void Free(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t Advance(
        EffectRegistrationTree kind, std::uint32_t cursor) = 0;
    virtual void Erase(EffectRegistrationTree kind,
                       std::uint32_t tree, std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t FirstElement(
        std::uint32_t tree) = 0;
};

class EffectSystemTreeCleanup final {
public:
    // IDA 0x63d3b0: disable auxiliary effect registration and drain both trees.
    [[nodiscard]] static std::uint8_t CleanupEffects(
        std::uint32_t auxiliary, IEffectSystemTreeCleanupRuntime& runtime);
};

}  // namespace pal4
