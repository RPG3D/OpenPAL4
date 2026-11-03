#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include "pal4/native_base_object_scene_frame.h"

namespace pal4 {
class NativePetManagerOwner;
class IScriptPetVisibilityRuntime;
class NativePlayerInitialResetActivity;
class NativeBaseObjectMemory;
class IGameObjectEmbeddedAnimationRuntime;
class NativePkFileManagerRuntime;
class NativePlayerTransformRuntime;
class NativeArenaCollisionTree;
class NativeEffectSceneSession;

// Scene ownership for a persistent pet. All constructor dependencies, including
// the arena/world, must outlive this session. Clear before effect/world release.
class NativePetSceneSession final {
public:
    NativePetSceneSession(const std::filesystem::path& root,
        NativePetManagerOwner&, NativePlayerInitialResetActivity&,
        NativeBaseObjectMemory&, IGameObjectEmbeddedAnimationRuntime&,
        std::uint32_t game, std::uint32_t arena, NativeArenaCollisionTree&,
        NativePkFileManagerRuntime&, NativeEffectSceneSession&,
        NativePlayerTransformRuntime&, std::uint32_t player_manager);
    ~NativePetSceneSession();
    NativePetSceneSession(const NativePetSceneSession&) = delete;
    NativePetSceneSession& operator=(const NativePetSceneSession&) = delete;
    // Original giPetShow's PAL4Pet/339 initialization; does not enable or enter.
    void LoadActor();
    [[nodiscard]] IScriptPetVisibilityRuntime& VisibilityRuntime() noexcept;
    [[nodiscard]] std::uint32_t Enter();
    [[nodiscard]] std::uint32_t Remove();
    // Original release order: scriptPetOnRemove then PetManager::Reset.
    void Clear();
    [[nodiscard]] NativeBaseObjectSceneFrameResult RenderFrame(std::uint32_t camera);
    [[nodiscard]] std::size_t OwnedActorCount() const noexcept;
    [[nodiscard]] bool AnimationBound() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}  // namespace pal4
