#include "native_combat_property_probe.h"
#include "pal4/native_combat_property_runtime.h"
#include "pal4/native_combat_inventory_events.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_combat_hints.h"
#include "pal4/native_game_timer_queue.h"
#include "pal4/native_cegui_text_conversion.h"
#include "pal4/use_property_task.h"
#include "pal4/native_combat_message_children.h"
#include "pal4/native_actor_instance_owner.h"
#include "pal4/gom_task_container.h"
#include "pal4/native_combat_command_actions.h"
#include "pal4/native_combat_event_handlers.h"
#include "pal4/native_combat_property_domain.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_player_action_memory.h"
#include "pal4/native_combat_death_actions.h"
#include "pal4/combat_end_action_task.h"
#include "pal4/game_resource_manager.h"
#include "../../native/ui/cegui_exports_native.h"
#include <algorithm>
#include <array>
#include <bit>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace pal4::test {
std::uint32_t FindNativeCombatPropertyForUi(NativeInventorySession& inventory,NativeSkillRecordCatalog& catalog){
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    for(std::size_t n=0;n<inventory.PropertyRecordCount();++n){const auto r=inventory.PropertyRecordAt(n),s=catalog.Address(word(r+148u));
        if(word(r+4u)!=0u || word(r+8u)>=6u || word(r+12u)!=1u || !s || word(s+16u)!=9118u)continue;
        const auto next=word(s+56u),extra=next?catalog.Address(next):0u;
        if(next && (!extra || word(extra+16u)!=9132u || word(extra+56u)))continue;
        if(std::bit_cast<float>(word(s+20u))>=1.f && (std::bit_cast<float>(word(s+32u))==2.f || std::bit_cast<float>(word(s+44u))==6.f)){
            std::cout<<"Property UI operand: actual ID="<<word(r)<<" original_AI="<<word(r+8u)<<" target="<<word(r+12u)<<" Skill="<<word(s)<<" root="<<word(s+16u)<<std::endl;return word(r);}
    }
    throw std::runtime_error("actual database lacks combat-usable single-target9118 recovery with bound Player9132 follow-up");
}
void VerifyNativeCombatPropertyEffects(NativeCombatSkillActorRegistry& registry,NativeInventorySession& inventory,NativeCombatRosterTargets& targets,
    NativeScriptEventRouter& router,NativeSkillRecordCatalog& catalog,std::span<NativeCombatActorSkillRuntime* const> players,NativeCombatHints& hints,NativeGameTimerQueue& timers){
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    check(!players.empty(),"Property effects probe requires actual Player");
    std::uint32_t property{},skill{};
    for(std::size_t n=0;n<inventory.PropertyRecordCount();++n){const auto r=inventory.PropertyRecordAt(n),s=catalog.Address(word(r+148u));
        if(word(r+4u)==0u && word(r+12u)<3u && s && word(s+16u)==9118u && !word(s+56u)){
            const auto probability=std::bit_cast<float>(word(s+20u));
            if(probability>=1.f && (std::bit_cast<float>(word(s+32u))==2.f || std::bit_cast<float>(word(s+44u))==6.f)){property=r;skill=s;break;}}
    }
    check(property && skill,"actual database has single-target recovery item with standalone9118");
    auto& source=*players.front();auto& target=*players.back();const auto a=source.ActorAddress(),b=target.ActorAddress(),id=word(property);
    const auto hp=target.CurrentStat(7u),mp=target.CurrentStat(9u);
    const auto count=inventory.ItemCount(id);
    struct Restore{NativeCombatActorSkillRuntime& target;std::int32_t hp,mp;~Restore(){(void)target.SetCurrentStat(7u,hp);(void)target.SetCurrentStat(9u,mp);}} restore{target,hp,mp};
    (void)target.SetCurrentStat(7u,1);(void)target.SetCurrentStat(9u,1);
    (void)inventory.AddItem(id,1); // Explicit isolated protocol operand, consumed once below.
    NativeCombatPropertyRuntime runtime(registry,inventory,targets,router);
    // Real3086 was rejected before its parent could run in live304, although
    // 17085 uses the already-composed9116 damage callback. Root admission must
    // not maintain a separate, incomplete copy of Skill dispatch capabilities.
    const auto firebead=inventory.PropertyAddress(3086u),fire_skill=catalog.Address(17085u);
    check(firebead && fire_skill && word(firebead+148u)==17085u && word(fire_skill+16u)==9116u,
          "original firebead3086 attaches actual17085 magic-damage root9116");
    runtime.ValidateDefinition(firebead);
    for(std::size_t n=0;n<inventory.PropertyRecordCount();++n){
        const auto definition=inventory.PropertyRecordAt(n),stock=inventory.ItemCount(word(definition));
        runtime.ValidateDefinition(definition);
        check(inventory.ItemCount(word(definition))==stock,
              "definition admission preserves original inventory across all attached-skill families");
    }
    std::cout<<"Native Property admission: all original owned definitions, including3086/17085/9116; no family filter or stock mutation; actual Skill execution remains separately validated\n";
    std::array<std::uint32_t,20> task{};task[0]=UsePropertyTask::kVtable;task[15]=a;task[16]=b;task[17]=property;task[18]=0xffffffffu;
    unsigned notified{},consumed{};
    const auto use=router.Subscribe(router.Key("ae_use_property"),3u,[&](auto payload){
        check(word(payload)==a+716u && word(payload+4u)==b && word(payload+8u)==property,"actual Property event packet order");
        bool rejected{};try{runtime.Process(task);}catch(const std::logic_error&){rejected=true;}check(rejected,"Property task pins its execution against reentry");++notified;});
    const auto consume=router.Subscribe(router.Key("add_property"),3u,[&](auto payload){
        check(notified==1u && word(payload)==id && word(payload+4u)==0xffffffffu,"original inventory event follows use notification and carries signed-1");++consumed;});
    struct Routes{NativeScriptEventRouter& r;std::uint32_t a,b;~Routes(){(void)r.Unsubscribe(a);(void)r.Unsubscribe(b);}} routes{router,use,consume};
    const auto reject=[&](const auto& operation){bool failed{};try{operation();}catch(const std::logic_error&){failed=true;}check(failed,"Property invalid operand rejected");};
    auto invalid=task;invalid[17]+=4u;reject([&]{runtime.Process(invalid);});
    invalid=task;invalid[16]+=4u;reject([&]{runtime.Process(invalid);});
    reject([&]{(void)NativeCombatInventoryEvents::Handle(0u,0u,inventory);});
    reject([&]{(void)NativeCombatInventoryEvents::Handle(0x57b0c0u,1u,inventory);});
    check(!notified && !consumed && inventory.ItemCount(id)==count+1u && target.CurrentStat(7u)==1 && target.CurrentStat(9u)==1,
          "foreign record/actor and malformed inventory event fail before use, stock and HP/MP mutation");
    runtime.Process(task);
    check(notified==1u && consumed==1u && inventory.ItemCount(id)==count && runtime.Complete(task),"original UseProperty consumes exactly one real item and empty effect finishes");
    check(target.CurrentStat(7u)>1 || target.CurrentStat(9u)>1,"actual attached9118 restores actual Combat Player HP/MP");
    // IDA55D062 asks the recipient's property slot12, independently of the
    // command slot8 that bans the recipient from using their own items.
    check(target.AddRecord(7651u,b),"original7651 applies ban to the recovery recipient");
    struct ClearBan{NativeCombatActorSkillRuntime& actor;~ClearBan(){actor.Remove(7651u);}} clear_ban{target};
    check(!target.CommandAvailability(4u) && target.AllowsDamage(4u),
          "banned recipient distinguishes command slot8 from incoming property slot12");
    (void)target.SetCurrentStat(7u,1);(void)target.SetCurrentStat(9u,1);
    (void)inventory.AddItem(id,1);notified=consumed=0u;
    runtime.Process(task);
    check(notified==1u && consumed==1u && inventory.ItemCount(id)==count && runtime.Complete(task),
          "recovery of banned recipient preserves real notification and inventory route");
    check(target.CurrentStat(7u)>1 || target.CurrentStat(9u)>1,
          "original property slot12 permits recovery while recipient has EAST_B3");
    target.Remove(7651u);
    std::cout<<"Native Property recipient gate: actual7651 B3 blocks own command4 but permits incoming recovery through property slot12\n";
    const auto owner=hints.Ensure(2u),root=hints.Root(2u);check(root && word(root+68u),"original ae_use_property shows actual DLL hint");
    const auto text=word(property+20u),size=word(property+24u);
    const auto expected=ConvertGameTextToUtf8(text?std::string_view(reinterpret_cast<const char*>(text),size):std::string_view{});check(expected.has_value(),"Property hint text converts");
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto get=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto cstr=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    check(std::string(cstr(get(reinterpret_cast<void*>(word(owner+40u)))))==*expected,"original Property name appears in real DLL hint");
    timers.Update(2.f);timers.Update(.001f);timers.Update(std::numeric_limits<float>::min());check(!word(root+68u),"Property hint original timer detaches popup");
    // Live300 rejected seal talisman before creating its parent because9000
    //was absent from admission, although the full skill dispatcher is bound.
    //Audit its real chain without touching application inventory/discovery.
    const auto seal=inventory.PropertyAddress(3057u),seal_skill=catalog.Address(7259u),status=catalog.Address(7313u),seal_count=inventory.ItemCount(3057u);
    check(seal && seal_skill && status && word(seal+148u)==7259u && word(seal_skill+16u)==9000u &&
          word(seal_skill+56u)==17089u && word(status+12u)==4u,"actual3057 seal chain and duration");
    runtime.ValidateDefinition(seal);
    check(word(b+636u)==0u,"seal fixture begins without wrapped status");
    struct RemoveSeal{NativeCombatActorSkillRuntime& target;~RemoveSeal(){target.Remove(7313u);}} remove_seal{target};
    check(source.TriggerForTarget(7259u,b) && word(b+636u)==2u,"real9000 applies7313 seal through target virtual268");
    const auto icons=target.StatusIconIds(false);
    check(std::find(icons.begin(),icons.end(),2)!=icons.end(),"actual7313 status icon is seal2");
    target.Advance(3u);check(word(b+636u)==2u,"seal remains before original four-turn resource limit");
    target.Advance(1u);check(word(b+636u)==0u,"seal expiry removes wrapped status through normal skill lifecycle");
    check(inventory.ItemCount(3057u)==seal_count,"direct seal skill audit preserves actual inventory quantity");
    std::cout<<"Native seal Property3057: admission9000, actual7259 ->7313/9100 +17089/9132, status2 and four-turn resource expiry pass; direct skill audit, no item consumption or parent equivalence claim\n";
    std::cout<<"Native Property terminal probe: actual ID="<<id<<" Skill="<<word(skill)<<" event/use -> inventory-1 -> real9118 HP="<<target.CurrentStat(7u)<<" MP="<<target.CurrentStat(9u)<<"; actual DLL name/timer, foreign/malformed/reentry guards and stack balance pass; parent animation/UI not part of this fixture\n";
}
void VerifyNativeCombatPropertyAnimation(NativeCombatSkillActorRegistry& registry,NativeInventorySession& inventory,
    NativeCombatRosterTargets& targets,NativeScriptEventRouter& router,NativeCombatActorSkillRuntime& source,
    NativeCombatActorSkillRuntime& target,NativeCombatMessageChildren::Model model,NativeEffectSceneSession& effects,
    ICombatActionTaskRuntime& playback,float& scale,NativeCombatPropertyActionRuntime::Motion motion){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto property=inventory.PropertyAddress(3012u),a=source.ActorAddress(),b=target.ActorAddress();
    check(property && model,"Property animation fixture requires original3012 and current-model resolver");
    const auto hp=target.CurrentStat(7u),mp=target.CurrentStat(9u);const auto count=inventory.ItemCount(3012u);
    struct Restore {NativeCombatActorSkillRuntime& target;std::int32_t hp,mp;~Restore(){(void)target.SetCurrentStat(7u,hp);(void)target.SetCurrentStat(9u,mp);}} restore{target,hp,mp};
    NativeCombatPropertyRuntime terminal(registry,inventory,targets,router);
    // Children destruct first, including partial StartAction failures.
    std::unique_ptr<NativeCombatPropertyActionRuntime> service;
    NativeCombatMessageChildren messages(source,playback,model,effects,router,scale);
    service=std::make_unique<NativeCombatPropertyActionRuntime>(a,registry,targets,effects,router,messages,terminal,motion);
    (void)target.SetCurrentStat(7u,1);(void)target.SetCurrentStat(9u,1);(void)inventory.AddItem(3012u,1);
    unsigned uses{},consumes{};
    const auto use=router.Subscribe(router.Key("ae_use_property"),3u,[&](auto p){if(*reinterpret_cast<const std::uint32_t*>(p)==a+716u)++uses;});
    const auto consume=router.Subscribe(router.Key("add_property"),3u,[&](auto p){if(*reinterpret_cast<const std::uint32_t*>(p)==3012u)++consumes;});
    struct Routes {NativeScriptEventRouter& r;std::uint32_t a,b;~Routes(){(void)r.Unsubscribe(a);(void)r.Unsubscribe(b);}} routes{router,use,consume};
    const auto child=messages.CreateProperty("z08",b,property,*service);
    auto& runtime=messages.ContainerRuntime();runtime.BeginRun(child);
    check(service->Subscriptions()==2u && messages.SubscriptionCount()==1u && !uses && !consumes && inventory.ItemCount(3012u)==count+1u,
          "original PropertyAction Start only subscribes/plays; inventory and effects wait for real AMF atk");
    unsigned frames{};for(;frames<600u && !runtime.IsDurationComplete(child);++frames){
        (void)model().Update(1.f/60.f,router);runtime.Tick(child,1.f/60.f);
    }
    check(frames && frames<600u && uses==1u && consumes==1u && inventory.ItemCount(3012u)==count &&
          (target.CurrentStat(7u)>1 || target.CurrentStat(9u)>1) && messages.Commands(),
          "original z08 AMF atk invokes real inventory/recovery once and naturally completes animation");
    runtime.Finish(child);check(!service->Subscriptions() && !messages.SubscriptionCount(),"PropertyAction Stop clears auxiliary/base event pairs");
    messages.Destroy(child);check(!messages.Count() && !messages.OwnedAllocations() && !messages.OwnedTexts(),"PropertyAction message and embedded task retirement balance");
    // Cancellation must not execute the pending atk or consume another item.
    const auto cancelled=messages.CreateProperty("z08",b,property,*service);
    runtime.BeginRun(cancelled);runtime.BeginRun(cancelled);
    check(service->Subscriptions()==2u && messages.SubscriptionCount()==1u && uses==1u && consumes==1u,
          "PropertyAction restart replaces owned subscriptions without executing pending atk");
    messages.Destroy(cancelled);
    bool retired_rejected{};try{service->Command(cancelled,"atk");}catch(const std::invalid_argument&){retired_rejected=true;}
    check(retired_rejected && inventory.ItemCount(3012u)==count && !messages.Count() && !messages.OwnedAllocations() &&
          !messages.OwnedTexts() && !service->Subscriptions() && !messages.SubscriptionCount(),
          "PropertyAction running cancellation retires callbacks/task/text; stale command rejected before mutation");
    messages.Close();motion(a,1u);
    std::cout<<"Native Property animation probe: actual3012/z08 -> original AMF atk -> inventory-1/9118; frames="<<frames
        <<" commands="<<messages.Commands()<<" uses="<<uses<<" consumes="<<consumes<<" HP="<<target.CurrentStat(7u)<<" MP="<<target.CurrentStat(9u)
        <<" restart/running-cancel/stale-command guards pass; auxiliary/base subscriptions and owned allocations/texts=0; parent/selector not part of this fixture\n";
}
void VerifyNativeCombatPropertyCommand(NativeCombatCommandActions& commands,NativeCombatEventHandlers& handlers,NativeInventorySession& inventory,
    NativeCombatActorSkillRuntime& source,NativeCombatActorSkillRuntime& target,NativeCombatPropertyDomain& domain,NativeCombatTaskQueue& queue,
    NativeCombatPlayerActionMemory& memory,NativeScriptEventRouter& router,NativeCombatDeathActions& death,NativeCombatMessageChildren::Model model,
    NativeCombatPropertyActionRuntime::Motion motion,const std::function<void()>& frame,const std::function<void()>& start_ui,std::uint32_t property_id,bool revive_target,bool attack_target){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    const auto a=source.ActorAddress(),b=target.ActorAddress(),property=inventory.PropertyAddress(property_id),count=inventory.ItemCount(property_id);
    check(property && !queue.Count() && !memory.Count() && !domain.Count(),"Property command requires idle actual queue/replay/domain");
    const auto hp=target.CurrentStat(7u),mp=target.CurrentStat(9u);const auto state=word(a+648u);
    const auto throws_before=domain.Throws(),updates_before=domain.ProjectileUpdates();
    struct Restore {NativeCombatActorSkillRuntime& source;NativeCombatActorSkillRuntime& target;std::int32_t hp,mp;std::uint32_t state;bool poison;
        ~Restore(){if(poison)target.Remove(17007u);(void)target.SetCurrentStat(7u,hp);(void)target.SetCurrentStat(9u,mp);*reinterpret_cast<std::uint32_t*>(source.ActorAddress()+648u)=state;}} restore{source,target,hp,mp,state,attack_target};
    bool rejected{};try{(void)domain.Create(property+4u,b);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && !domain.Count() && !domain.OwnedAllocations(),"foreign Property parent rejected before allocation");
    if(revive_target || attack_target){
        check(a!=b,"Property command fixture requires distinct actual actors");
        if(attack_target){
            check(source.IsPlayerActor() && !target.IsPlayerActor() && property_id==3069u && word(b+960u)==2503u,
                  "attack Property fixture retains actual3069 and original Monster2503 operands");
            std::cout<<"Native attack Property fixture begins: actual3069 -> Monster2503, full parent/Throw/terminal/EndAction required"<<std::endl;
            // Separate the actual attached Skill chain from the still-required
            // approach/projectile/terminal parent. No inventory operand is changed.
            check(hp==100,"original2503 starts with100HP in the isolated fixture");
            (void)source.TriggerForTarget(word(property+148u),b);
            const auto icons=target.StatusIconIds(true);
            check(target.CurrentStat(7u)==85 && std::find(icons.begin(),icons.end(),8)!=icons.end(),
                  "actual17080 ->9136/recovery/passive/effect chain applies15HP and fire poison");
            target.Remove(17007u);(void)target.SetCurrentStat(7u,hp);
            check(inventory.ItemCount(property_id)==count,"direct Skill audit does not consume or grant a Property");
            std::cout<<"Native attack Property attached Skill: actual17080/9136 ->17012/9118 ->17002/9000 ->17007 and17068/9132, Monster2503 HP100->85 and icon8, restored before full command fixture"<<std::endl;
        }
        // Fail at the same production admission gate as live101, before any
        // fixture HP/inventory mutation. A cold replay must remain independent.
        const auto cold=domain.Create(property,b);check(cold && domain.Owns(cold),"original revival Property accepted by actual domain");
        domain.Cancel(cold);
    }
    unsigned used{},ended{},alive{};
    const auto use=router.Subscribe(router.Key("ae_use_property"),3u,[&](auto p){if(word(p)==a+716u)++used;});
    const auto end=router.Subscribe(router.Key("ae_end_action"),3u,[&](auto p){if(word(p)==a+716u)++ended;});
    const auto living=router.Subscribe(router.Key("ae_become_alive"),3u,[&](auto p){if(word(p)==b+716u)++alive;});
    struct Routes {NativeScriptEventRouter& r;std::uint32_t a,b,c;~Routes(){(void)r.Unsubscribe(a);(void)r.Unsubscribe(b);(void)r.Unsubscribe(c);}} routes{router,use,end,living};
    if(!attack_target){(void)target.SetCurrentStat(7u,revive_target?0:1);(void)target.SetCurrentStat(9u,1);}
    (void)inventory.AddItem(property_id,1);
    if(revive_target)check(target.CurrentStat(7u)==0 && target.IsStateThree(),"revival fixture uses actual zero-HP dead Player");
    const std::array request{a+716u,b,property_id,2u};
    if(start_ui)start_ui();else (void)commands.Handle(handlers.ExtendedAddress(),reinterpret_cast<std::uint32_t>(request.data()));
    const auto saved=memory.Get(a);
    check(saved && domain.Owns(saved) && !queue.Contains(saved) && queue.Count()==1u && domain.Count()>=2u && !used && inventory.ItemCount(property_id)==count+1u,
          "original Property command creates independent queued and cold replay parents, no early use/stock mutation");
    unsigned frames{};while(queue.Count() && frames<900u){frame();++frames;}
    check(frames && frames<900u && !queue.Count() && !queue.PendingSuccessorCount() && used==1u && ended==1u &&
          inventory.ItemCount(property_id)==count && (attack_target || target.CurrentStat(7u)>1 || target.CurrentStat(9u)>1) && memory.Get(a)==saved && domain.Owns(saved),
          "actual Player Property command/parent/z08/AMF/recovery/EndAction naturally drains and preserves cold replay");
    memory.Clear();check(!domain.Count() && !domain.OwnedAllocations() && !domain.OwnedTexts() && !domain.Subscriptions(),"Property queue and replay independently retire all resources");
    if(attack_target){
        check(domain.Throws()==throws_before+1u && domain.ProjectileUpdates()>updates_before+1u,
              "attack Property creates and advances its actual palobject projectile");
        const auto icons=target.StatusIconIds(true);
        check(hp==100 && target.CurrentStat(7u)==85 && std::find(icons.begin(),icons.end(),8)!=icons.end(),
              "original3069 applies permanent17007 fire poison and its first15HP tick on actual2503");
        std::cout<<"Native attack Property command: actual3069 -> Monster2503 HP100->85, fire-poison icon8, uses="<<used
                 <<" end_events="<<ended<<" frames="<<frames<<" queue/replay/allocations/texts/subscriptions=0; isolated actual actor/task/effect fixture, not natural play"<<std::endl;
        // An explicit second fixture item exercises actual in-flight teardown.
        // Keep this standalone parent out of the production queue: cancelling
        // must neither synthesize EndAction nor execute the pending impact.
        const auto origin=*reinterpret_cast<const ResourceVec3*>(model().Address()+172u);
        struct RestoreFlightFixture {NativeInventorySession& inventory;std::uint32_t id;NativeCombatMessageChildren::Model model;ResourceVec3 origin;
            ~RestoreFlightFixture(){(void)inventory.RemoveItem(id,1);(void)model().SetPosition(origin);}} flight_fixture{inventory,property_id,model,origin};
        (void)inventory.AddItem(property_id,1);
        const auto cancelled=domain.Create(property,b);
        struct CancelFlight {NativeCombatPropertyDomain& owner;std::uint32_t parent;~CancelFlight(){if(owner.Owns(parent))owner.Cancel(parent);}} cancel_flight{domain,cancelled};
        domain.Prepare(cancelled);domain.BeginWaiting(cancelled);
        unsigned flight_frames{};
        while(domain.Throws()==throws_before+1u && flight_frames<600u){frame();domain.Update(cancelled,1.f/60.f);++flight_frames;}
        check(flight_frames && flight_frames<600u && domain.Throws()==throws_before+2u && !domain.Ready(cancelled) &&
              used==1u && ended==1u && inventory.ItemCount(property_id)==count+1u && target.CurrentStat(7u)==85,
              "second real projectile is in flight before use/stock/HP/EndAction mutation");
        check(domain.HiddenProjectilesOutsideWorld()==1u,
              "original55DE70 loads actual DGI with draw mode0 and never binds it to an RpWorld");
        domain.Cancel(cancelled);motion(a,1u);
        check(!domain.Count() && !domain.OwnedAllocations() && !domain.OwnedTexts() && !domain.Subscriptions() &&
              !queue.Count() && inventory.ItemCount(property_id)==count+1u && used==1u && ended==1u && target.CurrentStat(7u)==85,
              "in-flight cancellation deletes actual component and callbacks without impact or extra consumption");
        std::cout<<"Native attack Property in-flight cancel: frames="<<flight_frames
                 <<" actual projectile removed; stock/HP/use/EndAction unchanged; queue/domain/components/allocations/texts/subscriptions=0"<<std::endl;
        return;
    }
    if(revive_target){
        check(target.CurrentStat(7u)>0 && !target.IsStateThree() && alive==1u,"actual revival Skill chain emits one become-alive event and restores HP/state");
        std::cout<<"Native revival Property command: actual ID="<<property_id<<" HP="<<target.CurrentStat(7u)<<" alive_events="<<alive
            <<" uses="<<used<<" end_events="<<ended<<" frames="<<frames<<" queue/replay/allocations/texts/subscriptions=0; explicit dead-target fixture, not natural play\n";
        return;
    }
    // Complete another real animation, then exercise original source-state3
    // successor construction. This reversible operand does not claim death play.
    (void)inventory.AddItem(property_id,1);const auto parent=domain.Create(property,b);
    struct CancelParent {NativeCombatPropertyDomain& owner;std::uint32_t parent;~CancelParent(){if(owner.Owns(parent))owner.Cancel(parent);}} cancel_parent{domain,parent};
    domain.Prepare(parent);domain.BeginWaiting(parent);
    // Real Combat Player9132 now creates the original attachment. Advance the
    // supplied actor/effect/console frame, including its completion events.
    unsigned manual{};while(!domain.Ready(parent) && manual<600u){frame();domain.Update(parent,1.f/60.f);++manual;}
    check(domain.Ready(parent) && used==2u && inventory.ItemCount(property_id)==count,"death-successor fixture first completes actual Property animation");
    *reinterpret_cast<std::uint32_t*>(a+648u)=3u;check(source.IsStateThree(),"actual source property268 accepts reversible state3 operand");
    const auto dead=domain.ResultEvent(parent),next=word(dead+36u);
    check(dead && death.Owns(dead) && next && domain.Owns(next) && word(next)==CombatEndActionTask::kVtable,
          "original source3 completion owns actual Dead with EndAction in original result36 slot");
    domain.Cancel(parent);death.Cancel(dead);
    check(!domain.Count() && !domain.OwnedAllocations() && !domain.OwnedTexts() && !domain.Subscriptions(),"cancel pending Dead recursively retires its untransferred EndAction");
    *reinterpret_cast<std::uint32_t*>(a+648u)=state;motion(a,1u);
    std::cout<<"Native Player Property command protocol: actual ID="<<property_id<<" queued/replay -> parent/z08/atk/recovery -> EndAction frames="<<frames
        <<" uses="<<used<<" end_events="<<ended<<" queue/replay/allocations/texts/subscriptions=0; source3 actual Dead/EndAction construction and cancellation pass; selector="<<bool(start_ui)<<"\n";
}
}
