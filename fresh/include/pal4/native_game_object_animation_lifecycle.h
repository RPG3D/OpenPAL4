#pragma once

#include "pal4/game_object_embedded_animation.h"
#include "pal4/native_base_object_memory.h"

namespace pal4 {

// Owns the native lifetime of the embedded animation manager. SDK hierarchy
// lookup and PAL4's animation drawing remain supplied by a derived runtime.
class NativeGameObjectAnimationLifecycle :
    public IGameObjectEmbeddedAnimationRuntime {
public:
    explicit NativeGameObjectAnimationLifecycle(NativeBaseObjectMemory& memory)
        : memory_(memory) {}

    void InitializeAnimationData(
        GameObjectEmbeddedAnimationState& state) override;
    void CleanupAnimationData(
        GameObjectEmbeddedAnimationState& state) override;
    void CleanupAnimationDataRange(
        GameObjectEmbeddedAnimationState& state) override;
    std::uint32_t AnimationArrayCount(std::uint32_t address) override;
    void DestructAnimationArray(std::uint32_t address,
                                std::uint32_t element_size,
                                std::uint32_t count) override;
    std::uint32_t FreeMemory(std::uint32_t address) override;
    void FreeWrappedMemory(std::uint32_t address) override;

private:
    NativeBaseObjectMemory& memory_;
};

}  // namespace pal4
