#pragma once

#include "pal4/native_base_object_memory.h"
#include "pal4/game_object_embedded_animation.h"

#include <span>
#include <string_view>

namespace pal4 {

struct NativeBorrowedAnimationClip final {
    std::string_view action;
    std::uint32_t animation;
};

// Installs borrowed RenderWare pointers into an owned base-object image.
class NativeBorrowedActorBinding final {
public:
    // LoadClump normally owns label+4; borrowed assets still need that name
    // for scene/combat handoff. Its plain byte buffer uses the resource owner.
    static void BindLabel(NativeBaseObjectMemory& memory, std::uint32_t object,
                          std::string_view label);
    static void BindActor(NativeBaseObjectMemory& memory, std::uint32_t object,
                          std::uint32_t main_clump, std::uint32_t shadow_clump);
    // Preserve LoadClump's owned resource names and44288F's borrowed hook.
    static void BindModelInfo(NativeBaseObjectMemory&, std::uint32_t object,
                              std::string_view model, std::uint32_t bottom_hook);
    static void BindAnimations(
        NativeBaseObjectMemory& memory,
        IGameObjectEmbeddedAnimationRuntime& animation, std::uint32_t object,
        std::string_view base_path,
        std::span<const NativeBorrowedAnimationClip> clips);
    [[nodiscard]] static bool SelectAction(NativeBaseObjectMemory& memory,
                                            std::uint32_t object,
                                            std::string_view action);
    static void UnbindAnimationReferences(NativeBaseObjectMemory& memory, std::uint32_t object);
    static void Unbind(NativeBaseObjectMemory& memory, std::uint32_t object);
};

}  // namespace pal4
