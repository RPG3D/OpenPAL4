#pragma once

#include "pal4/native_monster_info_reader.h"
#include "pal4/native_monster_object_owner.h"

#include <filesystem>

namespace pal4 {

class NativeMonsterManagerOwner;

// Creates manager-owned monster images backed by decoded RenderWare assets.
class NativeMonsterSceneObjectBuilder final {
public:
    NativeMonsterSceneObjectBuilder(
        std::filesystem::path game_root, NativeMonsterManagerOwner& manager,
        NativeBaseObjectMemory& memory, IRwDictSchemaRuntime& schema,
        IGameObjectEmbeddedAnimationRuntime& animation,
        IGameResourceManagerRuntime& transforms);

    void Build(const NativeMonsterInfoRecord& record);

private:
    std::filesystem::path game_root_;
    NativeMonsterManagerOwner& manager_;
    NativeBaseObjectMemory& memory_;
    IRwDictSchemaRuntime& schema_;
    IGameObjectEmbeddedAnimationRuntime& animation_;
    IGameResourceManagerRuntime& transforms_;
};

}  // namespace pal4
