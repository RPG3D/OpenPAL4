#pragma once
#include "pal4/native_monster_encounter.h"
#include "pal4/native_floor_attribute_owner.h"
#include "pal4/native_floor_combat_areas.h"
#include "pal4/native_scene_player_movement.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_pal_game_iv_state_list.h"
#include "pal4/native_world_state_owner.h"
#include "pal4/native_script_combat_start.h"
#include "pal4/native_script_combat_config.h"
#include "pal4/native_event_document_runtime.h"

namespace pal4::test {
inline void VerifyMonsterEncounter(const std::filesystem::path& root,
    NativeBaseObjectMemory& memory, NativeMonsterManagerOwner& monsters,
    NativeSceneMovementAsset& movement, IMonsterProximityRuntime& proximity,
    std::uint32_t monster, std::uint32_t target) {
    const auto require=[](bool ok,const char* message) {
        if(!ok) throw std::runtime_error(message);
    };
    NativeGameDatabase database;
    require(database.Load(root),"encounter loads original CombatScene database");
    NativeFloorCombatAreas areas(database);
    NativeScenePlayerMovement player_movement(movement);
    NativeFloorAttributeOwner floor(player_movement);
    // ApplyColor only dispatches color callbacks. The actual alpha fields are
    // supplied explicitly here; production Update obtains them from movement.
    memory.WriteDword(floor.StateAddress(),25u);
    memory.WriteDword(floor.StateAddress()+4u,0u);
    require(floor.State().current_alpha==25u,"encounter fixture uses real floor owner alpha");
    NativeGameArenaManagerOwner arena;
    const auto arena_address=arena.Construct();
    const auto world=arena.AllocateQueueBytes(4u);
    std::memcpy(reinterpret_cast<void*>(world),"M01",4u);
    memory.WriteDword(arena_address+8u,world);
    NativePalGameIVTextureStorage game([](auto){});
    const auto game_address=game.Construct();
    NativeWorldStateOwner scene;
    const auto scene_address=scene.Construct();
    NativePalGameIVStateList states(game_address);
    require(states.Register(scene_address,1u,[]{}),"encounter registers actual Scene owner");
    memory.WriteDword(game_address+396u,scene_address);
    NativeEventDocumentRuntime documents(memory);
    NativeScriptCombatConfig config;
    ScriptCombatExecutor executor{0x1000u,{}};
    const auto source=memory.ReadDword(monster+0x654u);
    const auto count=memory.ReadDword(source+0x40u);
    const auto first_id=memory.ReadDword(memory.ReadDword(source+0x44u));
    unsigned entries{};
    NativeScriptCombatStart request(executor,documents,config,game_address,[&](auto document) {
        ++entries;
        require(documents.AttributeToString(documents.FindNumericAttribute(document,"CombatArea"))=="M01",
            "scene encounter resolves retained floor25 through real58-row catalog");
        require(documents.ChildCount(document)==count && config.Snapshot().words[2]==1u,
            "scene encounter dispatches original initiator records and owned name");
        require(documents.AttributeToInt(documents.FindNumericAttribute(
            documents.ChildAt(document,0u),"MonsterId"),-1)==static_cast<std::int32_t>(first_id),
            "scene encounter enters with original resource monster ID");
    });
    NativeMonsterEncounter encounter(monsters,memory,proximity,game,states,request,arena,floor,areas);
    memory.WriteDword(monster+0x6e4u,target);
    encounter.DispatchCombatPositions(monster,0.01f);
    require(!entries,"collection alone must not enter Combat");
    encounter.InitializeCombat(monster,-1);
    require(entries==1u && !request.Document() && !documents.DocumentCount(),
        "encounter bridge closes XML after synchronous actual writer callback");
    const auto* image=reinterpret_cast<const MonsterBehaviorManagerState*>(monster);
    require(image->words[0x714u/4u]==image->words[0x718u/4u] &&
        image->words[0x724u/4u]==image->words[0x728u/4u],
        "encounter consumes both pending logical ranges before Entry");
    unsigned names{};
    game.VisitStateMonsters([&](auto name) {
        ++names;
        require(name==std::string_view(reinterpret_cast<const char*>(memory.ReadDword(monster+4u))),
            "encounter preserves native instance label in game collection");
    });
    require(names==1u,"encounter only records current initiator when peers are idle");
    memory.WriteDword(game_address+396u,0u);
    states.Clear();scene.Destroy();game.Destroy();
    floor.Release();
    require(!floor.OutstandingAllocations(),"encounter releases real floor callbacks/storage");
    std::cout<<"PASS: native encounter bridge, original monster IDs, real floor/catalog and XML handoff\n";
}
}
