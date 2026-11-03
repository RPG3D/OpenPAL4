#pragma once

#include "pal4/effect_system_object_f.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IEffectSystemObjectFColorRuntime {
public:
    virtual ~IEffectSystemObjectFColorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t SetColor(
        std::uint32_t color_slot, std::uint32_t rgba) = 0;
};

class EffectSystemObjectFColorRuntimeStub final
    : public IEffectSystemObjectFColorRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t SetColor(std::uint32_t,
                                        std::uint32_t) override { return 0u; }
};

class EffectSystemObjectFColor final {
public:
    // IDA 0x606490. The +36 facet reads four bytes from the color slot.
    [[nodiscard]] static std::uint8_t Get(
        const EffectSystemObjectFState& state,
        std::array<std::uint8_t, 4>& rgba,
        IEffectSystemObjectFColorRuntime& runtime);
    // IDA 0x6064d0. Packs four pointed-to bytes in x86 byte order.
    [[nodiscard]] static std::uint8_t Set(
        const EffectSystemObjectFState& state,
        const std::array<std::uint8_t, 4>& rgba,
        IEffectSystemObjectFColorRuntime& runtime);
};

}  // namespace pal4
