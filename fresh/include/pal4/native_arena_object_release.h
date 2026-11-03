#pragma once
#include "pal4/game_arena_release.h"
#include <functional>
#include <memory>

namespace pal4 {
class NativeArenaNpcSceneLoader;
class NativeArenaNpcSceneEntry;
class NativeMonsterManagerOwner;
class NativeSceneGobAssets;
class NativeArenaGobSceneSession;
class NativeSceneEvfAssets;
class NativeWorldPathPlannerOwner;
class NativeSceneMovementAsset;
class NativeCameraManagerOwner;
class IBaseObjectMaterialRuntime;
class NativePalGameIVTextureStorage;

// Borrows the actual creating owners. Clear leaves empty owner objects alive;
// the caller may retire wrappers after the ordered release completes.
struct NativeArenaObjectReleaseContext {
    std::uint32_t arena;
    NativeArenaNpcSceneLoader& npcs;
    NativeArenaNpcSceneEntry& monster_entries;
    NativeMonsterManagerOwner& monsters;
    NativeSceneGobAssets& gobs;
    NativeArenaGobSceneSession& gob_entries;
    NativeSceneEvfAssets& evfs;
    NativeWorldPathPlannerOwner& paths;
    NativeSceneMovementAsset& movement;
    NativeCameraManagerOwner& cameras;
    std::function<void()> refresh_object_sprites;
    std::uint32_t game;
    IBaseObjectMaterialRuntime& npc_materials;
    NativePalGameIVTextureStorage& textures;
};

class NativeArenaObjectRelease final {
public:
    // Observer runs after each shared operation, for diagnostics and assertions.
    using Observer = std::function<void(GameArenaReleaseCall)>;
    static void Release(NativeArenaObjectReleaseContext context, Observer observer = {});
    static std::unique_ptr<IGameArenaReleaseRuntime> CreateRuntime(
        NativeArenaObjectReleaseContext context, Observer observer = {});
};
} // namespace pal4
