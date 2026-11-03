#pragma once

#include "pal4/effect_system.h"

#include <cstdint>

namespace pal4 {

class IEffectSystemSecondaryTimerRuntime {
public:
    virtual ~IEffectSystemSecondaryTimerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t NextPendingNode(
        std::uint32_t tree, std::uint32_t node);
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil(
        std::uint32_t tree) = 0;
    virtual void ResetDialog(std::uint32_t effect) = 0;
    virtual void ReturnEffect(std::uint32_t effect,
                              std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t EraseNode(
        std::uint32_t tree, std::uint32_t node) = 0;
};

class IEffectSystemObjectGroupRuntime {
public:
    virtual ~IEffectSystemObjectGroupRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void UpdateObject(std::uint32_t object,
                              std::uint32_t first,
                              std::uint32_t second) = 0;
};

}  // namespace pal4
