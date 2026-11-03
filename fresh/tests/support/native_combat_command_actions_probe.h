#pragma once
#include "pal4/native_combat_command_actions.h"
#include "pal4/native_combat_player_action_memory.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_attack_actions.h"
#include "pal4/native_combat_action_children.h"
#include "pal4/native_combat_event_handlers.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_script_event_router.h"
#include "pal4/check_protect_task.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatCommandActions(NativeCombatCommandActions& command,NativeCombatPlayerActionMemory& memory,
    NativeCombatTaskQueue& queue,NativeCombatAttackActions& actions,NativeCombatActionChildren& children,
    NativeCombatEventHandlers& handlers,NativeCombatActorSkillRuntime& source,std::uint32_t target,std::uint32_t other,std::uint32_t other_player,
    NativeCombatSystemImage& system,NativeScriptEventRouter& router,const NativeCombatActorSkillRuntime::ActionMotionRefresh& motion){
    const auto check=[](bool b,const char* message){if(!b)throw std::runtime_error(message);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    const auto owner=handlers.ExtendedAddress(),actor=source.ActorAddress();
    check(!queue.Count() && !memory.Count() && !actions.Count(),"command probe starts with idle actual domains");
    struct Restore {NativeCombatTaskQueue& queue;NativeCombatPlayerActionMemory& memory;NativeCombatSystemImage& system;
        std::uint32_t state;NativeScriptEventRouter& router;std::vector<std::uint32_t> routes;
        ~Restore(){for(auto h:routes)(void)router.Unsubscribe(h);queue.ResetForStart();memory.Clear();system.State()=state;}
    } restore{queue,memory,system,system.State(),router};
    std::array request{actor+716u,target,0u,4u};const auto payload=reinterpret_cast<std::uint32_t>(request.data());
    unsigned starts{},stops{},reentries{},command_starts{};bool fail_start{},in_command{};
    const auto dispatch=[&]{
        struct Scope{bool& b;Scope(bool& b):b(b){b=true;}~Scope(){b=false;}} scope(in_command);
        return command.Handle(owner,payload);
    };
    restore.routes.push_back(router.Subscribe(router.Key("ae_start_action"),1u,[&](std::uint32_t p){
        if(word(p)!=actor+716u)return;++starts;
        if(!in_command)return;++command_starts;
        bool rejected{};try{(void)command.Handle(owner,payload);}catch(const std::logic_error&){rejected=true;}
        check(rejected,"actual command callback cannot overwrite the active request");++reentries;
        if(fail_start)throw std::runtime_error("intentional command task preparation failure");
    }));
    restore.routes.push_back(router.Subscribe(router.Key("stop_select_target"),1u,[&](std::uint32_t p){
        check(word(p)==payload,"original57C4B0 stop event carries pointer to incoming request pointer");++stops;
    }));
    (void)dispatch();
    const auto saved=memory.Get(actor);
    std::cout<<"Native command creation snapshot: queued="<<queue.Count()<<" replay="<<memory.Count()
             <<" domain_tasks="<<actions.Count()<<" state="<<system.State()<<" stop_events="<<stops<<'\n';
    check(queue.Count()==1u && memory.Count()==1u && actions.Count()==2u && saved && !queue.Contains(saved) &&
          word(saved)==CheckProtectTask::kVtable && word(saved+44u)==actor && word(saved+48u)==target &&
          *reinterpret_cast<const std::uint8_t*>(saved+56u)==0u && word(actor+1316u)==saved &&
          *reinterpret_cast<const std::uint8_t*>(actor+1312u)==1u && word(actor+720u)==0u && system.State()==3u && stops==1u,
          "normal command owns independent queued/prepared and cold Player replay CheckProtect tasks");
    bool rejected{};try{memory.Assign(other_player,saved,actions);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && memory.Count()==1u && memory.Get(actor)==saved && !memory.Get(other_player),"two Players cannot own the same replay task");
    memory.Assign(actor,saved,actions);check(actions.Count()==2u && memory.Get(actor)==saved,"same template assignment retains original owning bit without deletion");
    const auto replacement=actions.Create(actor,other,0u);memory.Assign(actor,replacement,actions);
    check(!actions.Owns(saved) && actions.Count()==2u && memory.Get(actor)==replacement && word(replacement+48u)==other,
          "shared53A4A0 replacement destroys only old replay template");
    const auto known_queued=actions.Create(actor,other,0u);(void)queue.Register(known_queued);
    rejected=false;try{memory.Assign(actor,known_queued,actions);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && memory.Get(actor)==replacement && queue.Count()==2u,"queue-owned task cannot become a Player-owned template");
    queue.ResetForStart();check(actions.Count()==1u && memory.Get(actor)==replacement,"queue cleanup preserves Player replay template");
    memory.Clear();source.EndAction(motion);
    check(!actions.Count() && !actions.OwnedAllocations() && !actions.OwnedTexts() && !children.Count() && !word(actor+1316u),
          "both task ownership paths release actual containers, names, children and Player pointer");
    fail_start=true;rejected=false;try{(void)dispatch();}catch(const std::runtime_error&){rejected=true;}fail_start=false;
    check(rejected && !queue.Count() && !memory.Count() && !actions.Count() && !actions.OwnedAllocations() && stops==1u,
          "failed real queued preparation cancels unpublished task without a replay template or stop event");source.EndAction(motion);
    const auto state=system.State();const std::array monster{target+716u,actor,0u,4u};
    check(command.Handle(owner,reinterpret_cast<std::uint32_t>(monster.data()))==target && system.State()==state && !actions.Count(),
          "original non-Player command gate returns before state/task changes");
    const std::array foreign{0x1000u,target,0u,4u};rejected=false;
    try{(void)command.Handle(owner,reinterpret_cast<std::uint32_t>(foreign.data()));}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && !queue.Count() && !memory.Count() && command_starts==reentries && starts>=command_starts && stops==1u,"foreign facets reject and every command start callback rejects reentry");
    std::cout<<"Native combat command actions: original Player attack dispatch, separate queue/replay tasks, shared replacement, ownership rejection, real callback failure and zero allocations pass\n";
}
}
