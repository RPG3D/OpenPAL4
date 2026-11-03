#include "pal4/base_object_lifecycle.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t BaseObjectLifecycle::Construct(
    BaseObjectState& state, std::uint32_t address,
    IBaseObjectRuntime& runtime) {
    unsigned completed = 0u;
    try {
        runtime.ConstructResource(state, 0u);
        completed = 1u;
        runtime.InitializeAnimation(state, 0x170u);
        completed = 2u;
        state.Write(0x1ecu, 0x841e80u);
        state.Write(0x208u, 0x843434u);
        state.Write(0x20cu, 0u);
        state.Write(0x210u, 0x843428u);
        state.Write(0x214u, 0u);
        runtime.ConstructResource(state, 0x218u);
        completed = 3u;
        runtime.ConstructResource(state, 0x388u);
        completed = 4u;

        for (std::uint32_t offset = 0x4f8u; offset <= 0x510u;
             offset += 8u) {
            state.Write(offset, 0xffffffffu);
            state.Write(offset + 4u, 0u);
        }
        for (auto offset : {0x520u, 0x528u, 0x534u, 0x53cu}) {
            runtime.ConstructEffect(state, offset);
            ++completed;
        }
        runtime.ConstructText(state, 0x544u);
        completed = 9u;
        runtime.ConstructText(state, 0x554u);
        completed = 10u;
        runtime.ConstructCharacter(state, 0x564u);
        completed = 11u;

        state.Write(0u, 0x8433d8u);
        state.Write(0x170u, 0x8433c8u);
        state.Write(0x1ecu, 0x8433bcu);
        for (auto offset : {0x1f0u, 0x200u, 0x204u, 0x608u, 0x604u}) {
            state.Write(offset, 0u);
        }
        state.Write(0x214u, address);
        state.Write(0x20cu, address);
        for (auto offset : {0x518u, 0x51cu, 0x530u, 0x550u, 0x560u}) {
            state.Write(offset, 0u);
        }
        return address;
    } catch (...) {
        if (completed >= 11u) runtime.DestroyCharacter(state, 0x564u);
        if (completed >= 10u) runtime.CleanupText(state, 0x554u);
        if (completed >= 9u) runtime.CleanupText(state, 0x544u);
        constexpr std::uint32_t effect_offsets[]{
            0x520u, 0x528u, 0x534u, 0x53cu};
        for (unsigned index = completed < 8u ? completed : 8u;
             index > 4u; --index) {
            runtime.ResetEffect(state, effect_offsets[index - 5u]);
        }
        if (completed >= 4u) runtime.DestroyResource(state, 0x388u);
        if (completed >= 3u) runtime.DestroyResource(state, 0x218u);
        if (completed >= 2u) runtime.CleanupAnimation(state, 0x170u);
        if (completed >= 1u) runtime.DestroyResource(state, 0u);
        throw;
    }
}

void BaseObjectLifecycle::Destroy(
    BaseObjectState& state, IBaseObjectRuntime& runtime) {
    state.Write(0u, 0x8433d8u);
    state.Write(0x1ecu, 0x8433bcu);
    state.Write(0x170u, 0x8433c8u);
    runtime.DestroyCharacter(state, 0x564u);
    runtime.CleanupText(state, 0x554u);
    runtime.CleanupText(state, 0x544u);
    for (auto offset : {0x53cu, 0x534u, 0x528u, 0x520u}) {
        runtime.ResetEffect(state, offset);
    }
    runtime.DestroyResource(state, 0x388u);
    runtime.DestroyResource(state, 0x218u);
    state.Write(0x210u, 0x8425a0u);
    state.Write(0x208u, 0x8425a0u);
    state.Write(0x1ecu, 0x841e80u);
    runtime.CleanupAnimation(state, 0x170u);
    runtime.DestroyResource(state, 0u);
}

}  // namespace pal4
