#include "pal4/effect_system_object_f_collection.h"

namespace pal4 {
namespace {

bool Active(const EffectSystemObjectFState& state) {
    return (state.words[1u] & 0xffu) != 0u;
}

void ReleaseAll(EffectSystemObjectFState& state,
                IEffectSystemObjectFCollectionRuntime& runtime) {
    const auto begin = state.words[39u];
    for (auto cursor = begin; cursor != state.words[40u]; cursor += 4u) {
        const auto object = runtime.ReadWord(cursor);
        if (object != 0u) {
            runtime.Release(object);
            runtime.WriteWord(cursor, 0u);
        }
    }
    state.words[40u] = begin;
}

}  // namespace

bool EffectSystemObjectFCollection::Update(
    EffectSystemObjectFState& state, const std::uint32_t requested,
    IEffectSystemObjectFCollectionRuntime& runtime) {
    if (!Active(state))
        return false;
    if (state.words[39u] != 0u &&
        (state.words[40u] - state.words[39u]) / 4u != 0u)
        return true;
    for (std::uint32_t count = 0u; count < requested; ++count) {
        const auto object = runtime.HandleEvent(
            state, 56u, state.words[3u], state.words[4u]);
        if (object == 0u) {
            ReleaseAll(state, runtime);
            return false;
        }
        const auto begin = state.words[39u];
        const auto end = state.words[40u];
        if (end != state.words[41u]) {
            runtime.WriteWord(end, object);
            state.words[40u] = end + 4u;
            continue;
        }

        const auto size = begin == 0u ? 0u : (end - begin) / 4u;
        const auto growth = size > 1u ? size : 1u;
        const auto capacity = size + growth;
        const auto memory = runtime.Allocate(4u * capacity);
        for (std::uint32_t index = 0u; index < size; ++index)
            runtime.WriteWord(memory + 4u * index,
                              runtime.ReadWord(begin + 4u * index));
        runtime.WriteWord(memory + 4u * size, object);
        runtime.Free(begin);
        state.words[39u] = memory;
        state.words[40u] = memory + 4u * (size + 1u);
        state.words[41u] = memory + 4u * capacity;
    }
    return true;
}

std::uint8_t EffectSystemObjectFCollection::Clear(
    EffectSystemObjectFState& state,
    IEffectSystemObjectFCollectionRuntime& runtime) {
    if (!Active(state))
        return static_cast<std::uint8_t>(state.words[1u]);
    const auto old_end = state.words[40u];
    ReleaseAll(state, runtime);
    return static_cast<std::uint8_t>(old_end);
}

std::uint32_t EffectSystemObjectFCollection::Get(
    const EffectSystemObjectFState& state, const std::uint32_t index,
    IEffectSystemObjectFCollectionRuntime& runtime) {
    if (!Active(state) || state.words[39u] == 0u)
        return 0u;
    const auto size = (state.words[40u] - state.words[39u]) / 4u;
    if (index >= size)
        return 0u;
    return runtime.ReadWord(state.words[39u] + 4u * index);
}

}  // namespace pal4
