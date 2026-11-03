#pragma once

#include "pal4/effect_object.h"

#include <cstdint>

namespace pal4 {

class IEffectObjectRenderRuntime {
public:
    virtual ~IEffectObjectRenderRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t EffectManager() = 0;
    [[nodiscard]] virtual std::uint8_t ValidateTransform(
        std::uint32_t manager, std::uint32_t context,
        std::uint32_t transform) = 0;
    // IDA610A17 calls nullsub_3(context). Nested leaf payloads are drawn by
    // the group queues; recursively rendering this Dialog would draw twice.
    virtual void NoOpDraw(std::uint32_t context) = 0;
};

class EffectObjectRenderRuntimeStub final : public IEffectObjectRenderRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetObjectTransform(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t EffectManager() override { return 0u; }
    [[nodiscard]] std::uint8_t ValidateTransform(
        std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    void NoOpDraw(std::uint32_t) override {}
};

class EffectObjectRenderer final {
public:
    // IDA 0x6109a0.
    static void Render(const EffectObjectState& state, std::uint32_t context,
                       IEffectObjectRenderRuntime& runtime);
};

}  // namespace pal4
