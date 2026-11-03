#include "pal4/strip_collection.h"

namespace pal4 {
namespace {

bool Active(const StripObjectState& state) {
    return (state.words[1u] & 0xffu) != 0u;
}

void ReleaseAll(StripObjectState& state, IStripCollectionRuntime& runtime) {
    const auto begin = state.words[72u];
    for (auto cursor = begin; cursor != state.words[73u]; cursor += 4u) {
        const auto object = runtime.ReadWord(cursor);
        if (object != 0u) {
            runtime.Release(object);
            runtime.WriteWord(cursor, 0u);
        }
    }
    state.words[73u] = begin;
}

}  // namespace

bool StripCollection::Update(
    StripObjectState& state, const std::uint32_t requested,
    IStripCollectionRuntime& runtime) {
    if (!Active(state))
        return false;
    const auto begin = state.words[72u];
    if (begin != 0u && (state.words[73u] - begin) / 4u != 0u)
        return true;
    for (std::uint32_t count = 0u; count < requested; ++count) {
        const auto object = runtime.HandleEvent(
            state, state.words[39u], state.words[3u], state.words[4u]);
        if (object == 0u) {
            ReleaseAll(state, runtime);
            return false;
        }
        const auto old_begin = state.words[72u];
        const auto end = state.words[73u];
        if (end != state.words[74u]) {
            runtime.WriteWord(end, object);
            state.words[73u] = end + 4u;
            continue;
        }

        const auto size = old_begin == 0u ? 0u : (end - old_begin) / 4u;
        const auto growth = size > 1u ? size : 1u;
        const auto capacity = size + growth;
        const auto memory = runtime.Allocate(4u * capacity);
        for (std::uint32_t i = 0u; i < size; ++i)
            runtime.WriteWord(memory + 4u * i,
                              runtime.ReadWord(old_begin + 4u * i));
        runtime.WriteWord(memory + 4u * size, object);
        runtime.Free(old_begin);
        state.words[72u] = memory;
        state.words[73u] = memory + 4u * (size + 1u);
        state.words[74u] = memory + 4u * capacity;
    }
    return true;
}

std::uint8_t StripCollection::Clear(
    StripObjectState& state, IStripCollectionRuntime& runtime) {
    if (!Active(state))
        return static_cast<std::uint8_t>(state.words[1u]);
    const auto old_end = state.words[73u];
    ReleaseAll(state, runtime);
    return static_cast<std::uint8_t>(old_end);
}

std::uint32_t StripCollection::Get(
    const StripObjectState& state, const std::uint32_t index,
    IStripCollectionRuntime& runtime) {
    if (!Active(state) || state.words[72u] == 0u)
        return 0u;
    const auto size = (state.words[73u] - state.words[72u]) / 4u;
    if (index >= size)
        return 0u;
    return runtime.ReadWord(state.words[72u] + 4u * index);
}

}  // namespace pal4
