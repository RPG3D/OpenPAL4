#pragma once

#include "pal4/strip_object.h"

#include <cstdint>

namespace pal4 {

class IStripRenderRuntime {
public:
    virtual ~IStripRenderRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t EffectManager() = 0;
    [[nodiscard]] virtual std::uint8_t ValidateTransform(
        std::uint32_t manager, std::uint32_t context,
        std::uint32_t translation) = 0;
    [[nodiscard]] virtual std::uint8_t RenderStraight(
        std::uint32_t model, std::uint32_t caller_ebp,
        std::uint32_t caller_edi) = 0;
    [[nodiscard]] virtual std::uint8_t RenderClump(
        std::uint32_t clump) = 0;
};

class StripRenderer final {
public:
    // IDA 0x619b80.
    [[nodiscard]] static std::uint8_t Render(
        const StripObjectState& state, std::uint32_t context,
        std::uint32_t caller_ebp, std::uint32_t caller_edi,
        IStripRenderRuntime& runtime);
};

}  // namespace pal4
