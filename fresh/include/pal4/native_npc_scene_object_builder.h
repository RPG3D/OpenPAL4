#pragma once

#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_npc_object_owner.h"
#include "pal4/native_npc_info_reader.h"

#include <filesystem>
#include <memory>
#include <string>

namespace pal4 {

class NativeNpcManagerOwner;

// Builds one NPC from a loaded npcInfo record before scene callbacks run.
class NativeNpcSceneObjectBuilder final {
public:
    NativeNpcSceneObjectBuilder(
        std::filesystem::path game_root, NativeNpcManagerOwner& manager,
        NativeBaseObjectMemory& memory, IRwDictSchemaRuntime& schema,
        IGameObjectEmbeddedAnimationRuntime& animation,
        IGameResourceManagerRuntime& transforms);

    [[nodiscard]] bool Build(
        const NativeNpcInfoRecord& record,
        std::unique_ptr<NativeAnimatedActorAsset>& actor,
        std::unique_ptr<NativeNpcObjectOwner>& object);
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    std::filesystem::path game_root_;
    NativeNpcManagerOwner& manager_;
    NativeBaseObjectMemory& memory_;
    IRwDictSchemaRuntime& schema_;
    IGameObjectEmbeddedAnimationRuntime& animation_;
    IGameResourceManagerRuntime& transforms_;
    std::string last_error_;
};

}  // namespace pal4
