#include "pal4/cylinder_object_collection.h"

namespace pal4 {
namespace {

bool Active(const CylinderObjectState& state) {
    return (state.words[1u] & 0xffu) != 0u;
}

void ReleaseAll(CylinderObjectState& state,
                ICylinderObjectCollectionRuntime& runtime) {
    const auto begin = state.words[36u];
    for (auto cursor = begin; cursor != state.words[37u]; cursor += 4u) {
        const auto object = runtime.ReadWord(cursor);
        if (object != 0u) {
            runtime.Release(object);
            runtime.WriteWord(cursor, 0u);
        }
    }
    state.words[37u] = begin;
}

}  // namespace

bool CylinderObjectCollection::Update(
    CylinderObjectState& state, const std::uint32_t requested,
    ICylinderObjectCollectionRuntime& runtime) {
    if (!Active(state))
        return false;
    const auto begin = state.words[36u];
    if (begin != 0u && (state.words[37u] - begin) / 4u != 0u)
        return true;

    for (std::uint32_t count = 0u; count < requested; ++count) {
        const auto object = runtime.HandleEvent(
            state, state.words[23u], state.words[3u], state.words[4u]);
        if (object == 0u) {
            ReleaseAll(state, runtime);
            return false;
        }
        const auto old_begin = state.words[36u];
        const auto old_end = state.words[37u];
        if (old_end != state.words[38u]) {
            runtime.WriteWord(old_end, object);
            state.words[37u] = old_end + 4u;
            continue;
        }

        const auto size = old_begin == 0u ? 0u : (old_end - old_begin) / 4u;
        const auto growth = size > 1u ? size : 1u;
        const auto capacity = size + growth;
        const auto memory = runtime.Allocate(4u * capacity);
        for (std::uint32_t index = 0u; index < size; ++index)
            runtime.WriteWord(memory + 4u * index,
                              runtime.ReadWord(old_begin + 4u * index));
        runtime.WriteWord(memory + 4u * size, object);
        runtime.Free(old_begin);
        state.words[36u] = memory;
        state.words[37u] = memory + 4u * (size + 1u);
        state.words[38u] = memory + 4u * capacity;
    }
    return true;
}

std::uint8_t CylinderObjectCollection::Clear(
    CylinderObjectState& state, ICylinderObjectCollectionRuntime& runtime) {
    if (!Active(state))
        return static_cast<std::uint8_t>(state.words[1u]);
    const auto old_end = state.words[37u];
    ReleaseAll(state, runtime);
    return static_cast<std::uint8_t>(old_end);
}

std::uint32_t CylinderObjectCollection::Get(
    const CylinderObjectState& state, const std::uint32_t index,
    ICylinderObjectCollectionRuntime& runtime) {
    if (!Active(state) || state.words[36u] == 0u)
        return 0u;
    const auto size = (state.words[37u] - state.words[36u]) / 4u;
    if (index >= size)
        return 0u;
    return runtime.ReadWord(state.words[36u] + 4u * index);
}

}  // namespace pal4
