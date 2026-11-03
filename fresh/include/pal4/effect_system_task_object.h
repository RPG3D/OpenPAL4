#pragma once

#include <cstddef>
#include <cstdint>

#include "pal4/gom_task_base.h"

namespace pal4 {

struct EffectSystemTaskObjectState {
    GomTaskBaseState base;             // +0..+59
    std::uint8_t enabled{};           // +60
    std::uint8_t copy_timing{};       // +61
    std::uint8_t padding[2]{};
    std::uint32_t reserved64{};       // +64
    std::uint32_t property68{};       // +68
};
static_assert(sizeof(EffectSystemTaskObjectState) == 72u);
static_assert(offsetof(EffectSystemTaskObjectState, enabled) == 60u);
static_assert(offsetof(EffectSystemTaskObjectState, copy_timing) == 61u);
static_assert(offsetof(EffectSystemTaskObjectState, reserved64) == 64u);
static_assert(offsetof(EffectSystemTaskObjectState, property68) == 68u);

class EffectSystemTaskObject final {
public:
    static constexpr std::uint32_t kVtable = 0x84b298u;

    EffectSystemTaskObject(GomTaskBase& base,
                           IGomTaskBaseRuntime& runtime)
        : base_(base), runtime_(runtime) {}

    // IDA 0x63fc90: construct the 72-byte shared effect-task base.
    EffectSystemTaskObjectState& Construct(
        EffectSystemTaskObjectState& state);
    // IDA 0x63fcb0/0x63fcd0: deleting and ordinary destruction.
    [[nodiscard]] std::uint32_t Delete(
        EffectSystemTaskObjectState& state,
        std::uint32_t object_address, std::uint32_t flags);
    [[nodiscard]] std::uint32_t Cleanup(
        EffectSystemTaskObjectState& state);
    // IDA 0x63fcf0: copy timing from the linked task when flag +61 is set.
    [[nodiscard]] std::uint8_t SetupState(
        EffectSystemTaskObjectState& state);

private:
    GomTaskBase& base_;
    IGomTaskBaseRuntime& runtime_;
};

}  // namespace pal4
