#include "pal4/game_object_animation_collection.h"

#include <algorithm>

namespace pal4 {

namespace {

template <typename Matches>
void RemoveMatching(GameObjectEmbeddedAnimationState& state,
                    IGameObjectAnimationCollectionRuntime& runtime,
                    Matches matches) {
    auto& words = state.words;
    auto cursor = words[7];
    while (cursor != words[8]) {
        if (matches(runtime.ReadPointer(cursor))) {
            for (auto source = cursor + 4u; source != words[8]; source += 4u)
                runtime.WritePointer(source - 4u,
                                     runtime.ReadPointer(source));
            words[8] -= 4u;
        } else {
            cursor += 4u;
        }
    }
}

}  // namespace

bool GameObjectAnimationCollection::AddOrReplace(
    GameObjectEmbeddedAnimationState& state,
    const std::uint32_t animation,
    IGameObjectAnimationCollectionRuntime& runtime) {
    RemoveMatching(state, runtime, [&](const std::uint32_t stored) {
        return runtime.SameObjectKey(stored, animation);
    });
    auto& words = state.words;
    const auto begin = words[7];
    const auto end = words[8];
    if (words[9] - end >= 4u) {
        runtime.WritePointer(end, animation);
        words[8] = end + 4u;
        return true;
    }

    const auto size = begin == 0 ? 0u : (end - begin) / 4u;
    const auto capacity = size + std::max(size, 1u);
    const auto allocation = runtime.Allocate(capacity * 4u);
    if (allocation == 0) return false;
    for (std::uint32_t index = 0; index < size; ++index)
        runtime.WritePointer(allocation + index * 4u,
                             runtime.ReadPointer(begin + index * 4u));
    runtime.WritePointer(allocation + size * 4u, animation);
    runtime.Free(begin);
    words[7] = allocation;
    words[8] = allocation + (size + 1u) * 4u;
    words[9] = allocation + capacity * 4u;
    return true;
}

bool GameObjectAnimationCollection::RemoveByKey(
    GameObjectEmbeddedAnimationState& state,
    const std::uint32_t key,
    IGameObjectAnimationCollectionRuntime& runtime) {
    RemoveMatching(state, runtime, [&](const std::uint32_t stored) {
        return runtime.MatchesKey(stored, key);
    });
    return true;
}

}  // namespace pal4
