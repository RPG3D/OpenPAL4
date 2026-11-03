#include "native_combat_player_magic_command_probe.h"
#include "pal4/native_combat_command_actions.h"
#include "pal4/native_combat_event_handlers.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_skill_domain.h"
#include "pal4/native_magic_window_records.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_player_action_memory.h"
#include "pal4/native_script_event_router.h"
#include <array>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
void VerifyNativeCombatPlayerMagicCommand(NativeCombatCommandActions& commands,NativeCombatEventHandlers& handlers,
    std::span<NativeCombatActorSkillRuntime* const> players,NativeCombatMonsterManager& monsters,const NativeMagicWindowRecords& records,
    NativeCombatTaskQueue& queue,NativeCombatPlayerActionMemory& memory,NativeScriptEventRouter& router,
    const std::function<NativeCombatSkillDomain&(std::uint32_t)>& resolve,const std::function<void()>& frame,
    const std::function<bool(NativeCombatActorSkillRuntime&,std::uint32_t,std::uint32_t)>& start_ui,
    const std::function<std::string_view(std::uint32_t)>& animation_name){
    const auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
    const auto* record=records.FindMagic(5000u);check(record!=nullptr,"Magic5000 exists in actual database");
    const auto cost=*reinterpret_cast<const std::int32_t*>(reinterpret_cast<const char*>(record)+76u);
    NativeCombatActorSkillRuntime* source{};for(auto* s:players)if(s && s->CurrentStat(9u)>=cost){source=s;break;}
    check(source && !queue.Count() && !memory.Count(),"Player Magic protocol fixture requires idle actual queue/memory and sufficient real MP");
    std::uint32_t target{};for(std::size_t n=0;n<monsters.Count();++n){auto a=monsters.ActorAt(n);if(monsters.Skills(a).ReadyForAction() && monsters.Skills(a).AiTargetId()==2580){target=a;break;}}
    check(target!=0u,"Player Magic protocol fixture borrows actual M01 minion");
    const auto actor=source->ActorAddress();
    const auto before_mp=source->CurrentStat(9u),before_hp=monsters.Skills(target).CurrentStat(7u);
    auto& domain=resolve(actor);const auto casts_before=domain.Casts();unsigned ended{};
    const auto subscription=router.Subscribe(router.Key("ae_end_action"),3u,[&](std::uint32_t event){if(*reinterpret_cast<const std::uint32_t*>(event)==actor+716u)++ended;});
    struct Unsubscribe{NativeScriptEventRouter& r;std::uint32_t id;~Unsubscribe(){(void)r.Unsubscribe(id);}} unsubscribe{router,subscription};
    bool rebind_rejected=false;try{commands.BindMagic(records,resolve);}catch(const std::logic_error&){rebind_rejected=true;}
    check(rebind_rejected,"Player Magic rejects replacement of borrowed record/domain services");
    check(!records.FindMagic(0xffffffffu),"missing Magic fixture ID remains outside actual catalog");
    const std::array<std::uint32_t,4> missing{actor+716u,target,0xffffffffu,0u};
    check(commands.Handle(handlers.ExtendedAddress(),reinterpret_cast<std::uint32_t>(missing.data()))==0u &&
          !queue.Count() && !memory.Count() && source->CurrentStat(9u)==before_mp && monsters.Skills(target).CurrentStat(7u)==before_hp,
          "original missing Magic lookup returns before tasks, MP or HP changes");
    const std::array<std::uint32_t,4> request{actor+716u,target,5000u,0u};
    const bool from_ui=start_ui && start_ui(*source,target,5000u);
    if(!from_ui)(void)commands.Handle(handlers.ExtendedAddress(),reinterpret_cast<std::uint32_t>(request.data()));
    const auto saved=memory.Get(actor);
    check(saved && domain.Owns(saved) && !queue.Contains(saved) && queue.Count() && domain.Count()>=2u,
          "original Player Magic command creates separate queued and cold replay parents");
    bool cast_animation_seen{};
    unsigned frames{};while(queue.Count() && frames<900u){frame();++frames;
        const auto name=animation_name(actor);cast_animation_seen|=name=="Z07" || name=="z07";
    }
    check(cast_animation_seen,"actual Player magic uses virtual404 Z07 rather than the physical attack fallback");
    check(!queue.Count() && ended && domain.Casts()==casts_before+1u && source->CurrentStat(9u)==before_mp-cost && domain.Owns(saved) && memory.Get(actor)==saved,
          "actual Player Magic animation/cost/EndAction drain queue and preserve cold replay template");
    const auto after_hp=monsters.Skills(target).CurrentStat(7u);
    memory.Clear();check(!domain.Count() && !domain.OwnedAllocations() && !domain.OwnedTexts() && !memory.Count(),
          "Player Magic protocol fixture retires both actual ownership paths");
    std::cout<<"Native Player Magic command protocol: actual DB5000 fixture, source="<<actor<<" target="<<target<<" frames="<<frames
        <<" MP="<<before_mp<<"->"<<source->CurrentStat(9u)<<" HP="<<before_hp<<"->"<<after_hp
        <<" EndAction="<<ended<<" queue/replay allocations=0; learned_selector="<<from_ui<<" (direct protocol fallback when actual learned collection lacks5000)\n";
}
}
