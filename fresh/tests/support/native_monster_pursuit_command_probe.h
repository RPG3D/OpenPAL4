#pragma once
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_monster_pursuit_runtime.h"
#include "pal4/native_monster_manager_owner.h"
#include "pal4/native_monster_pursuit_path_runtime.h"
#include "pal4/collision_actor.h"
#include "pal4/native_script_player_lock_runtime.h"
#include <algorithm>
#include <vector>
#include <utility>

namespace pal4::test {
inline void VerifyNativeMonsterPursuitCommand(NativeMonsterManagerOwner& monsters,
    NativeScriptMonsterPursuitRuntime& runtime,NativeBaseObjectMemory& memory,
    NativeArenaCollisionTree& tree, NativeScriptPlayerLockRuntime& lock,
    std::uint32_t controlled_actor) {
    const auto check=[](bool ok){if(!ok) throw std::runtime_error("native stop-pursuit command probe");};
    {
        NativeMonsterPursuitPathRuntime paths(monsters, tree, memory);
        CollisionActorState detached{};
        detached.words[3u] = 0x42f60000u;
        detached.words[10u] = 1u;
        const auto before = detached.words;
        const auto address = reinterpret_cast<std::uint32_t>(&detached);
        check(!paths.ApplyPursuitPath(address, {1,2,3}, {4,5,6}, 0u, 0u));
        check(detached.words == before);
        bool rejected = false;
        try { (void)paths.SetPursuitSpeed(address, 50.0f); }
        catch (const std::logic_error&) { rejected = true; }
        check(rejected);
        detached.words[1u] = 1u;
        rejected = false;
        try { (void)paths.ApplyPursuitPath(address, {1,2,3}, {4,5,6}, 0u, 0u); }
        catch (const std::logic_error&) { rejected = true; }
        check(rejected);
        std::cout << "Monster detached pursuit: original null-tree rejection preserves image; foreign tree/speed still guarded\n";
    }
    std::vector<std::pair<std::uint32_t,std::uint32_t>> saved;
    monsters.VisitObjects([&](std::uint32_t object) {
        // This probe preserves idle monsters. Active return is exercised by
        // the real path/pursuit tests and production scene adapter.
        check(memory.ReadDword(object+0x6e0u)!=1u);
        saved.emplace_back(object,memory.ReadDword(object+0x704u));
        memory.WriteDword(object+0x704u,0u);return true;
    });
    check(!saved.empty());
    const auto allocations=memory.OutstandingAllocations();
    const auto manager=monsters.Address();
    NativeAngelScriptEngine engine;
    NativeScriptFrameRunner runner(engine);
    constexpr char source[]="void Stop() { giMonsterStopPursuit(); giMonsterStopPursuit(); } "
        "void Lock() { giPlayerLock(); } void Unlock() { giPlayerUnLock(); }";
    check(engine.Raw()->AddScriptSection("monster_stop","monster_stop.cs",source,sizeof(source)-1)>=0 &&
        engine.Raw()->Build("monster_stop")>=0);
    NativeGameScriptServices services{};services.monster_pursuit=&runtime;services.player_lock=&lock;
    NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    runner.Queue("monster_stop","Stop");check(runner.Tick(0.0f)==NativeScriptFrameStatus::finished);
    for(const auto& [object,flag]:saved) {
        check(memory.ReadDword(object+0x704u)==1u);memory.WriteDword(object+0x704u,flag);
    }
    check(monsters.Address()==manager && monsters.ObjectCount()==saved.size() &&
        memory.OutstandingAllocations()==allocations);
    std::cout<<"Monster stop-pursuit: actual SDK repeated command sets every original monster flag and preserves ownership\n";
    const auto selected=std::find_if(saved.begin(),saved.end(),[&](const auto& entry) {
        return memory.ReadDword(entry.first+0x730u)!=1u &&
            memory.ReadDword(entry.first+0x568u)==tree.TreeAddress();
    });
    check(selected!=saved.end() && controlled_actor);
    const auto chaser=selected->first;
    (void)monsters.EnterOwnedPursuit(chaser,controlled_actor);
    check(memory.ReadDword(chaser+0x6e0u)==1u);
    runner.Queue("monster_stop","Lock");check(runner.Tick(0.0f)==NativeScriptFrameStatus::finished);
    check(memory.ReadDword(chaser+0x704u)==1u && memory.ReadDword(chaser+0x6e0u)!=1u &&
          memory.ReadDword(chaser+0x6e4u)==0u);
    const auto locked_allocations=memory.OutstandingAllocations();
    runner.Queue("monster_stop","Lock");check(runner.Tick(0.0f)==NativeScriptFrameStatus::finished);
    check(memory.OutstandingAllocations()==locked_allocations);
    runner.Queue("monster_stop","Unlock");check(runner.Tick(0.0f)==NativeScriptFrameStatus::finished);
    check(memory.ReadDword(chaser+0x704u)==0u);
    (void)monsters.ResetOwnedPursuit(chaser);
    for(const auto& [object,flag]:saved) memory.WriteDword(object+0x704u,flag);
    check(monsters.Address()==manager && monsters.ObjectCount()==saved.size());
    std::cout<<"Player lock active pursuit: real owned pursuit entry, compiled lock twice, actual return path, target clearing, no repeated allocations and unlock/fixture restoration pass\n";

}
}
