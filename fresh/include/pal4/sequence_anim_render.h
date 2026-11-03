#pragma once

#include "pal4/sequence_anim_object.h"

#include <cstdint>

namespace pal4 {

class ISequenceAnimRenderRuntime {
public:
    virtual ~ISequenceAnimRenderRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t EffectManager() = 0;
    [[nodiscard]] virtual std::uint8_t ValidateTransform(
        std::uint32_t manager, std::uint32_t context,
        std::uint32_t transform) = 0;
    [[nodiscard]] virtual std::uint8_t RenderResource(
        std::uint32_t resource, std::uint32_t caller_ebp,
        std::uint32_t caller_edi) = 0;
};

class SequenceAnimRenderRuntimeStub final : public ISequenceAnimRenderRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetObjectTransform(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t EffectManager() override { return 0u; }
    [[nodiscard]] std::uint8_t ValidateTransform(
        std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t RenderResource(
        std::uint32_t, std::uint32_t, std::uint32_t) override { return 0u; }
};

class SequenceAnimRenderer final {
public:
    // IDA 0x616360. Caller EBP/EDI are forwarded by the native render tail.
    [[nodiscard]] static std::uint8_t Render(
        const SequenceAnimObjectState& state, std::uint32_t context,
        std::uint32_t caller_ebp, std::uint32_t caller_edi,
        ISequenceAnimRenderRuntime& runtime);
};

}  // namespace pal4
