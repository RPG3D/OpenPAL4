#pragma once
#include "pal4/native_monster_interaction.h"
#include "pal4/native_new_game_script_session.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_scene_movement_asset.h"
#include "pal4/native_arena_npc_scene_entry.h"
#include "pal4/monster_interaction_processor.h"
#include <array>
#include <bit>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
// Explicit layout fixture around original models and original SDK bytecode.
// Never modifies either live game or the source resource files.
inline void VerifyMonsterInteraction(const std::filesystem::path& root,
    NativeBaseObjectMemory& memory, NativeMonsterManagerOwner& monsters,
    NativeArenaNpcSceneEntry& entries, NativeSceneMovementAsset& movement,
    const GameMathTables& math, IMonsterProximityRuntime& proximity,
    std::uint32_t* control, std::uint32_t monster, std::uint32_t target) {
    const auto require=[](bool ok,const char* message) {
        if(!ok) throw std::runtime_error(message);
    };
    const auto source=memory.ReadDword(monster+0x654u);
    auto* config=const_cast<BinaryXmlConfig*>(monsters.InfoDocument(memory.ReadDword(source+0x4cu)));
    require(config!=nullptr,"interaction fixture requires owned buffer cache");
    const auto original=*config;
    const auto count=memory.ReadDword(source+0x40u);
    const auto range=control[0x24u/4u],busy=control[0x2dcu/4u];
    struct Restore {
        BinaryXmlConfig& config; BinaryXmlConfig saved;
        NativeBaseObjectMemory& memory; std::uint32_t source,count;
        std::uint32_t* control; std::uint32_t range,busy;
        ~Restore() { config=std::move(saved);memory.WriteDword(source+0x40u,count);
            control[0x24u/4u]=range;control[0x2dcu/4u]=busy; }
    } restore{*config,original,memory,source,count,control,range,busy};
    const auto set_script=[&](const std::string& function) {
        const auto encoded=BinaryXmlConfig::Serialize(BinaryXmlNode{"fixture",{
            {"AAB_MST_ESSBEH_script_func",3u,function}}});
        require(encoded && config->Parse(*encoded),"interaction config fixture encoding");
    };
    NativeGameArenaManagerOwner arena;
    const auto arena_address=arena.Construct();
    const auto world=arena.AllocateQueueBytes(4u);
    std::memcpy(reinterpret_cast<void*>(world),"M06",4u);
    memory.WriteDword(arena_address+8u,world);
    arena.SetSceneName("3A");
    require(arena.QueueSceneScript([](const auto&){return true;}),"prepare arena script names");
    NativePkFileManagerRuntime packages;
    std::array<std::uint32_t,20> filesystem{};filesystem[10]=0x1010u;
    packages.ProcessPackFiles(packages.InitializePackFileManager(),
        reinterpret_cast<std::uint32_t>(filesystem.data()));
    NativeNewGameScriptSession scripts(root,{});
    unsigned clears{},missing{};
    NativeMonsterInteraction interaction({memory,monsters,arena,entries,scripts,
        packages,math,proximity,reinterpret_cast<std::uint32_t>(control),{},
        [&]{++clears;},[&]{++missing;}});
    control[0x24u/4u]=std::bit_cast<std::uint32_t>(1000000.0f);
    control[0x2dcu/4u]=1u;
    require(!interaction.Process(monster,target) && !clears,"busy actor must not queue monster script");
    control[0x2dcu/4u]=0u;
    set_script("");
    require(interaction.Process(monster,target) && !scripts.TimedScriptsActive(),
        "scriptless original-model interaction returns encounter eligibility");
    set_script("M06_3A_init");
    require(interaction.Process(monster,target),"scripted monster with IDs retains encounter eligibility");
    const auto module=std::string(arena.SceneScriptPath())+"[M06_3A_init]";
    require(scripts.HasExternalStackModule(module) && clears==1u && !missing &&
        scripts.TimedScriptsStarted()==1u && scripts.TimedScriptsFinished()==0u,
        "actual original bytecode is queued once without inline execution");
    require(interaction.Process(monster,target) && clears==1u && scripts.TimedScriptsStarted()==1u,
        "active monster module must not be duplicated");
    set_script("__missing_monster_function_fixture__");
    bool rejected=false;
    try {
        // Pursuit uses Runtime directly. An absent optional entry must release
        // its package scope without replacing the previously queued context.
        (void)MonsterInteractionProcessor::Process(
            *reinterpret_cast<MonsterBehaviorManagerState*>(monster),monster,target,
            interaction.Runtime());
    } catch(const std::exception&) { rejected=true; }
    require(!rejected && scripts.HasExternalStackModule(module) && clears==1u &&
        scripts.TimedScriptsStarted()==1u && scripts.TimedScriptsFinished()==0u,
        "absent optional function preserves the queued context and its waits");
    set_script("M06_3A_init");
    require(interaction.Process(monster,target) && scripts.TimedScriptsStarted()==1u,
        "direct pursuit interaction retains duplicate guard after absent optional entry");
    memory.WriteDword(source+0x40u,0u);
    auto* clump=reinterpret_cast<RpClump*>(memory.ReadDword(monster+0x14u));
    require(movement.IsObjectClumpRegistered(clump),"monster collision initially registered");
    require(!interaction.Process(monster,target) && !movement.IsObjectClumpRegistered(clump),
        "zero-ID scripted monster retires movement through creating scene entry");
    scripts.ReleaseServices();
    require(!scripts.HasExternalStackModule(module),"queued monster context released before owners");
    arena.ClearActiveSceneRecords();
    packages.Shutdown();
    std::cout<<"PASS: original-model monster interaction, real queued CSB, duplicate guard and movement retirement\n";
}
}
