#pragma once

#include "pal4/game_object_embedded_animation.h"

#include <cstdint>

namespace pal4 {

class IGameObjectAnimationCollectionRuntime {
public:
    virtual ~IGameObjectAnimationCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadPointer(
        std::uint32_t address) = 0;
    virtual void WritePointer(std::uint32_t address,
                              std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool SameObjectKey(
        std::uint32_t stored, std::uint32_t incoming) = 0;
    [[nodiscard]] virtual bool MatchesKey(
        std::uint32_t stored, std::uint32_t key) = 0;
};

class GameObjectAnimationCollection final {
public:
    [[nodiscard]] static bool AddOrReplace(
        GameObjectEmbeddedAnimationState& state,
        std::uint32_t animation,
        IGameObjectAnimationCollectionRuntime& runtime);
    [[nodiscard]] static bool RemoveByKey(
        GameObjectEmbeddedAnimationState& state,
        std::uint32_t key,
        IGameObjectAnimationCollectionRuntime& runtime);
};

}  // namespace pal4
