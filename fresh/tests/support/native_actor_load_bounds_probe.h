#pragma once
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_monster_scene_object_builder.h"
#include "pal4/native_monster_manager_owner.h"
#include "pal4/native_npc_scene_object_builder.h"
#include "pal4/native_npc_manager_owner.h"
#include "pal4/native_player_transform_runtime.h"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNative472LoadBounds(const std::filesystem::path& root,
    NativeBaseObjectMemory& memory, IRwDictSchemaRuntime& schema,
    IGameObjectEmbeddedAnimationRuntime& animation) {
    // Original pair308 m15-04-04, model472: cached resource sphere before
    // animation selection. All31 source Atomic spheres match the original.
    constexpr std::array<std::uint32_t,4> original{1051861032u,1108763089u,1077514088u,1112710225u};
    NativeAnimatedActorAsset actor;
    if(!actor.Load(root,"472"))throw std::runtime_error("472 load-bounds asset: "+actor.LastError());
    if(!std::equal(original.begin(),original.end(),actor.InitialBounds().begin()))
        throw std::runtime_error("472 asset initial sphere differs from original cached sphere");
    NativePlayerTransformRuntime transforms;
    GameResourceManager posed;
    posed.words[5]=reinterpret_cast<std::uint32_t>(actor.Main());
    (void)posed.CalculateBoundingBox(transforms);
    if(posed.words[55]==original[3])
        throw std::runtime_error("472 fixture must distinguish load pose from first animation pose");
    actor.Unload();
    NativeMonsterInfoReader reader;
    if(!reader.Load(root,"M15","1"))throw std::runtime_error("M15 load-bounds monster info");
    const auto record=std::find_if(reader.Records().begin(),reader.Records().end(),
        [](const auto& value){return value.name=="m15-04-04";});
    if(record==reader.Records().end() || record->resource!="472")
        throw std::runtime_error("M15 original472 scene record");
    NativeMonsterManagerOwner manager;
    if(!manager.Construct())throw std::runtime_error("472 scene manager");
    NativeMonsterSceneObjectBuilder builder(root,manager,memory,schema,animation,transforms);
    if(!manager.InitializeScene(std::span<const NativeMonsterInfoRecord>(&*record,1),
        [&](const auto& value){builder.Build(value);}))
        throw std::runtime_error("472 scene object creation");
    const auto object=manager.FindObject(record->name);
    if(!object)throw std::runtime_error("472 scene object registration");
    for(unsigned i=0;i<original.size();++i) {
        const auto actual=memory.ReadDword(object+(52u+i)*4u);
        std::cout<<"472 scene cached sphere word"<<i<<'='<<actual<<" expected="<<original[i]<<'\n';
        if(actual!=original[i])throw std::runtime_error("472 scene builder cached animated sphere");
    }
    std::cout<<"PASS: actual M15 m15-04-04 scene builder preserves original load sphere\n";

    NativeNpcInfoReader npc_reader;
    if(!npc_reader.Load(root,"Q01","N01") || npc_reader.Records().size()!=1u)
        throw std::runtime_error("original Q01/N01 NPC load-bounds record");
    std::unique_ptr<NativeAnimatedActorAsset> npc_asset;
    std::unique_ptr<NativeNpcObjectOwner> npc_object;
    NativeNpcManagerOwner npc_manager;
    if(!npc_manager.Construct() || !npc_manager.LoadInfo(npc_reader.Records()) ||
       !npc_manager.InitializeSceneGrid())throw std::runtime_error("NPC load-bounds manager");
    NativeNpcSceneObjectBuilder npc_builder(root,npc_manager,memory,schema,animation,transforms);
    if(!npc_builder.Build(npc_reader.Records().front(),npc_asset,npc_object))
        throw std::runtime_error("NPC load-bounds builder: "+npc_builder.LastError());
    for(unsigned i=0;i<4;++i)
        if(memory.ReadDword(npc_object->Address()+(52u+i)*4u)!=npc_asset->InitialBounds()[i])
            throw std::runtime_error("NPC scene builder replaced initial sphere");
    std::cout<<"PASS: actual Q01/N01 NPC scene builder preserves asset load sphere\n";
}
}
