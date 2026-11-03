#pragma once

#include "pal4/effect_system.h"

#include <cstdint>

namespace pal4 {

class IEffectSystemTimedRuntime {
public:
    virtual ~IEffectSystemTimedRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil(
        std::uint32_t tree) = 0;
    virtual void InsertTimedNode(std::uint32_t tree,
                                 std::uint32_t nil,
                                 std::uint32_t parent,
                                 std::uint32_t effect,
                                 float lifetime) = 0;
    virtual void UpdateTimedObject(std::uint32_t effect,
                                   double seconds) = 0;
    virtual void DrawTimedObject(std::uint32_t effect,
                                 std::uint32_t context) = 0;
    virtual void BeforeDestroyEffect(std::uint32_t effect) = 0;
    virtual void DestroyEffect(std::uint32_t effect,
                               std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t EraseTimedNode(
        std::uint32_t tree, std::uint32_t node) = 0;
};

}  // namespace pal4
