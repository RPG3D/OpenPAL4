#pragma once
#include "pal4/native_combat_actor_effects.h"
#include "pal4/native_combat_actor_retirement.h"
#include "pal4/native_combat_event_handlers.h"
#include "pal4/native_combat_actor_selection.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_actor_instance_pool.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_actor_user_data.h"
#include "pal4/native_effect_handle_content_runtime.h"
#include "pal4/game_resource_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_player_collection_values.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/random_number_generator.h"
#include "native_monster_target_selection_probe.h"
#include <bit>
#include <cmath>
#include <iostream>
#include <rpworld.h>
namespace pal4::test {
inline void VerifyNativeCombatActorEffects(NativeEffectSceneSession& effects,NativeScriptEventRouter& router,
    const std::filesystem::path& root,RpWorld* combat_world) {
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    const auto world_clumps=[&]{unsigned count{};RpWorldForAllClumps(combat_world,
        [](RpClump* clump,void* p)->RpClump*{++*static_cast<unsigned*>(p);return clump;},&count);return count;};
    check(combat_world && !world_clumps(),"selection begins with a separate empty Combat world");
    std::array<std::uint32_t,128> game{};
    NativeGameDatabase database;check(database.Load(root),"Combat effect Monster database");
    NativeMonsterRecordCatalog records(database);
    NativeActorInstancePool pool([]{return false;},[&](std::string_view model){
        auto child=std::make_unique<NativeActorInstanceOwner>();
        child->Construct(address(game.data()));child->Load(root,model);return child;
    });
    NativeCombatActorModelOwner owner(pool);const auto actor=owner.Construct(records,2579);
    NativeCombatActorEffects attachments(owner,effects,router);
    struct Drain {
        NativeEffectSceneSession& effects;NativeScriptEventRouter& router;
        std::uint32_t subscription{};RpClump* clump{};RwFrame* frame{};
        ~Drain(){
            if(subscription)(void)router.Unsubscribe(subscription);
            if(frame)NativeActorUserData::RemoveFrame(clump,"hook_ef_bottom",frame);
            try{effects.ClearCombatEffects();}catch(...){}
        }
    } drain{effects,router};
    const auto absent=attachments.Create("foot_2","hook_area");
    check(absent.id==0xffffffffu && !absent.owner,"52E2E0 without child returns invalid and creates nothing");
    {
        NativeCombatActorSelection cold_selection(owner,effects,router);bool rejected=false;
        try{(void)cold_selection.SetSelected(1u);}catch(const std::logic_error&){rejected=true;}
        check(rejected,"unloaded selection rejects before creating effects");cold_selection.Close();
    }
    owner.SetSlot(0u);(void)owner.RefreshResource();
    const auto* model=owner.Model();check(model && model->Asset()->Main(),"Combat effect real SDK model");
    // The original facet reports stored child coordinates and tracks movement.
    auto* mutable_model=const_cast<NativeActorInstanceOwner*>(model);
    mutable_model->SetRotation({7,13,19});
    unsigned attached{};
    drain.subscription=router.Subscribe(router.Key("ae_attach_effect"),1u,[&](std::uint32_t p){
        const auto* payload=reinterpret_cast<const std::uint32_t*>(p);
        check(payload[0]==actor+716u && payload[2]==effects.CombatEffectCreation().Group(),
              "52E2E0 attach event has actual actor facet716 and Combat handle owner");
        check(EffectHandle::DialogNeedsUpdate({payload[1],payload[2]},effects.ContentHandles()),
              "attach event receives a live native effect");
        bool rejected=false;try{(void)attachments.Create("foot_2","hook_area");}catch(const std::logic_error&){rejected=true;}
        check(rejected,"recursive attachment is rejected before creating another effect");++attached;
    });
    const auto dialog=[&](EffectSystemHandleWords h)->DialogObjectState& {
        const auto node=effects.ContentHandles().FindElement(h.system+12u,h);
        return *reinterpret_cast<DialogObjectState*>(effects.ContentHandles().ReadWord(node+16u));
    };
    const NativeDialogProperties::Reader reader=[&](std::uint32_t facet){return attachments.ReadPositionFacet(facet);};
    for(unsigned branch=0;branch<3u;++branch) {
        const auto mode=branch==0u?"HOOK_AREA":branch==1u?"hook_ef_pos":"hook_ef_bottom";
        const auto created=attachments.Create("foot_2",mode);
        EffectSystemHandleWords h{created.id,created.owner};
        check(EffectHandle::DialogNeedsUpdate(h,effects.ContentHandles()),"52E2E0 real original foot_2 handle");
        auto& state=dialog(h);
        const auto f=[&](unsigned index){return std::bit_cast<float>(state.words[index]);};
        std::array<float,3> vector{};
        if(branch==0u) {
            check(f(44)==41 && f(45)==0 && f(46)==-148,"hook_area uses original Monster fixed slot448 descriptor");
        } else if(branch==1u) {
            check(f(44)==-92 && f(45)==2 && f(46)==-102 && f(50)==7 && f(51)==13 && f(52)==19,
                  "hook_ef_pos uses child position/Y2 and actual child160 rotation vector");
        } else {
            check(f(50)==90,"ordinary hook adds90 degrees aroundX");
            auto* hook=NativeActorUserData::FindAtomic(model->Asset()->Main(),mode);
            check(hook && RwFrameGetParent(reinterpret_cast<RwFrame*>(state.words[43]))==RpAtomicGetFrame(hook),
                  "ordinary Combat hook attaches actual effect frame via original8451 userdata");
            drain.clump=model->Asset()->Main();drain.frame=reinterpret_cast<RwFrame*>(state.words[43]);
        }
        if(branch) {
            check(effects.CombatEffectVector(h,"T1",vector,reader) && vector==attachments.ReadPositionFacet(actor+724u),
                  "T1 is a borrowed live actor724 facet");
            (void)mutable_model->SetPosition({-92,5,-102});
            check(effects.CombatEffectVector(h,"T1",vector,reader) && vector[1]==5,"T1 observes subsequent SDK child movement");
        }
        effects.UpdateCombatEffects(0.04);
        if(drain.frame){NativeActorUserData::RemoveFrame(drain.clump,"hook_ef_bottom",drain.frame);drain.frame=nullptr;}
        effects.ReleaseCombatEffect(h,true);
        check(!h.system && !effects.CombatActiveCount(),"Combat attachment release retires actual active Dialog");
        (void)mutable_model->SetPosition({-92,0,-102});
    }
    check(attached==3u,"exactly three actual attachment events");
    check(router.Unsubscribe(drain.subscription),"Combat attachment observer retired");drain.subscription=0u;
    {
        NativeCombatActorSelection selection(owner,effects,router);
        unsigned selected{},unselected{};bool throw_select{};
        const auto reject=[&](auto&& call){bool rejected=false;try{call();}catch(const std::logic_error&){rejected=true;}check(rejected,"selection callback guard");};
        const auto handle=[&](unsigned offset){return *reinterpret_cast<EffectSystemHandleWords*>(actor+offset);};
        const auto picked=router.Subscribe(router.Key("ae_ui_select"),1u,[&](std::uint32_t p){
            check(*reinterpret_cast<const std::uint32_t*>(p)==actor+716u,"selection emits actual actor facet");
            check(EffectHandle::DialogNeedsUpdate(handle(852u),effects.ContentHandles()) &&
                  EffectHandle::DialogNeedsUpdate(handle(860u),effects.ContentHandles()),"both pick and OM03 exist before selection event");
            reject([&]{(void)selection.SetSelected(0u);});reject([&]{selection.Close();});
            reject([&]{owner.Actor().Destroy();});++selected;
            if(throw_select)throw std::logic_error("intentional selection observer failure");
        });
        const auto unpicked=router.Subscribe(router.Key("ae_ui_unselect"),1u,[&](std::uint32_t p){
            check(*reinterpret_cast<const std::uint32_t*>(p)==actor+716u && !handle(852u).system && !handle(860u).system,
                  "unselect cleans actual handles before notification");++unselected;
        });
        struct RetireSelectionObservers {NativeScriptEventRouter& router;std::uint32_t a,b;
            ~RetireSelectionObservers(){(void)router.Unsubscribe(a);(void)router.Unsubscribe(b);}} observers{router,picked,unpicked};
        for(unsigned cycle=0;cycle<3u;++cycle) {
            (void)selection.SetSelected(1u);
            auto& pick=dialog(handle(852u));auto& marker=dialog(handle(860u));
            const auto f=[](const DialogObjectState& state,unsigned word){return std::bit_cast<float>(state.words[word]);};
            const auto expected_scale=static_cast<float>(static_cast<double>(*reinterpret_cast<const float*>(model->Address()+620u))*0.025);
            const auto top=*reinterpret_cast<const float*>(model->Address()+524u+28u);
            // Original XML uses uppercase PICK although the resource request
            // in52B570 is lowercase pick.
            check(EffectHandle::GetName(handle(852u),effects.ContentHandles())=="PICK" &&
                  EffectHandle::GetName(handle(860u),effects.ContentHandles())=="OM03",
                  "selection loads original pick and OM03 resources");
            check(f(marker,44)==-92.0f && f(marker,45)==top+20.0f && f(marker,46)==-102.0f,
                  "OM03 uses actual child position and bounding-box top plus20");
            check(f(pick,47)==expected_scale && f(pick,48)==expected_scale && f(pick,49)==expected_scale,
                  "pick uses actual Monster child620 scale times0.025");
            std::array<float,3> value{};
            check(effects.CombatEffectVector(handle(852u),"T1",value,reader) && value==attachments.ReadPositionFacet(actor+724u),
                  "actual pick keeps live actor724 position facet");
            effects.UpdateCombatEffects(0.04);
            check(world_clumps()>0u,"selection model attaches to actual CombatSystem116 SDK world");
            NativeParticleProbeCamera selection_camera;selection_camera.Begin();
            effects.DrawCombatEffects(0u);(void)selection_camera.EndPixels();
            (void)selection.SetSelected(0u);
            check(!effects.CombatActiveCount(),"selection cleanup retires both active Dialogs");
            effects.ClearCombatEffects(); // Drain the stopped pick's pending playback.
            check(!world_clumps(),"selection cleanup removes Combat model clumps before world retirement");
        }
        throw_select=true;reject([&]{(void)selection.SetSelected(1u);});throw_select=false;
        check(handle(852u).system && handle(860u).system,"observer failure preserves original preceding writes for cleanup");
        (void)selection.SetSelected(0u);effects.ClearCombatEffects();
        (void)selection.SetSelected(1u);selection.Close();selection.Close();effects.ClearCombatEffects();
        reject([&]{(void)selection.SetSelected(1u);});
        check(selected==5u && unselected==4u && !handle(852u).system && !handle(860u).system,
              "selection repeated calls/throw recovery/Close preserve event counts and release handles");
        std::cout<<"Native52B570 selection: original pick/OM03, real B01 bounds/scale/facet, 5 selects/4 unselects, callback guards, actual Combat world attachment/frame update/draw/removal and effect cleanup pass\n";
    }
    std::cout<<"Native52E2E0: real B01/foot_2, no-child, three hook branches, actor724 live facet, child160 rotation, SDK hook frame and ae_attach_effect verified\n";
}
inline void VerifyNativeCombatActorEntry(NativeEffectSceneSession& effects,NativeScriptEventRouter& router,
    const std::filesystem::path& root,std::uint32_t combat) {
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    NativeGameDatabase database;check(database.Load(root),"entry original database");
    NativeMonsterRecordCatalog records(database);NativeSkillRecordCatalog catalog(database);
    NativePlayerCollectionCatalog passive(database);NativePlayerCombatEvents events;
    RandomNumberGenerator random;NativeSkillSystemOwner system(random,events);
    (void)system.Construct("H:/PAL4/objs");
    std::array<std::uint32_t,128> game{};
    NativeActorInstancePool pool([]{return false;},[&](std::string_view name){
        auto child=std::make_unique<NativeActorInstanceOwner>();
        child->Construct(reinterpret_cast<std::uint32_t>(game.data()));child->Load(root,name);return child;
    });
    NativeCombatMonsterManager manager(pool,records,catalog,router,system);
    std::vector<std::string> order;
    std::vector<std::uint32_t> observers;
    for(const auto* event:{"me_join_stage","me_join_scene","ae_state_changed","ae_become_alive","ae_attach_effect"}) {
        observers.push_back(router.Subscribe(router.Key(event),1u,[&,event](std::uint32_t payload) {
            const auto a=*reinterpret_cast<const std::uint32_t*>(payload)-716u;
            check(manager.ActorAt(manager.Count()-1u)==a,"entry event retains actual actor facet");
            bool blocked=false;try{manager.InitializeMonster(a,0u,passive,effects);}catch(const std::logic_error&){blocked=true;}
            check(blocked,"entry rejects recursive initialization before mutation");
            blocked=false;try{manager.Close();}catch(const std::logic_error&){blocked=true;}
            check(blocked,"entry pins manager across event callbacks");
            order.emplace_back(event);
        }));
    }
    // Exercise the guarded retirement also used by application shutdown.
    const auto drain=[&] {
        for(std::size_t i=0;i<manager.Count();++i) {
            const auto a=manager.ActorAt(i);
            manager.RetireEntryEffects(a,effects);
            manager.RetireEntryEffects(a,effects);
            for(const auto offset:{868u,876u,884u}) {
                check(!reinterpret_cast<const EffectSystemHandleWords*>(a+offset)->system,
                      "entry retirement clears handles and is repeatable");
            }
        }
    };
    try {
        for(unsigned i=0;i<3;++i) {
            const auto a=manager.CreateMonster(i?2580:2579,5u);
            order.clear();
            *reinterpret_cast<std::uint8_t*>(a+892u)=1u;
            manager.InitializeMonster(a,i,passive,effects);
            check(manager.ModelOwner(a).Slot()==i && *reinterpret_cast<const std::uint32_t*>(a+332u)==i &&
                *reinterpret_cast<const std::uint32_t*>(a+668u)==i,"entry sets mirrored formation slot");
            check(!*reinterpret_cast<const std::uint8_t*>(a+892u) &&
                *reinterpret_cast<const std::uint32_t*>(a+828u)==1u &&
                !*reinterpret_cast<const std::uint32_t*>(a+832u),"entry resets original state fields");
            check(manager.StageCount()==i+1u && manager.ModelOwner(a).Model() &&
                manager.ModelOwner(a).Model()->Asset()->CurrentAction()=="Z01",
                "entry executes actual join/model/animation sequence");
            const auto h=*reinterpret_cast<const EffectSystemHandleWords*>(a+868u);
            check(EffectHandle::DialogNeedsUpdate(h,effects.ContentHandles()),"entry original B01/B02 bottom is a live effect");
            std::cout<<"Native52CFD0 event order:";for(const auto& name:order)std::cout<<' '<<name;std::cout<<'\n';
            //531450 ends with CombatActor_UpdateHealthStatus, which already
            // emits alive before531720.52CFD0 then explicitly calls548 again.
            check(order==std::vector<std::string>{"ae_state_changed","ae_become_alive","me_join_stage","me_join_scene",
                "ae_state_changed","ae_become_alive","ae_attach_effect"},
                "entry preserves rebuild health, join, explicit alive and attachment event order");
            std::cout<<"Native52CFD0 monster="<<(i?2580:2579)<<" model="<<manager.ModelOwner(a).Model()->ModelName()
                     <<" bottom="<<h.id<<","<<h.system<<"\n";
        }
        check(!manager.ActiveIterators(),"entry motion retires stage cursors");
        effects.UpdateCombatEffects(0.04);
        drain();for(const auto h:observers)(void)router.Unsubscribe(h);observers.clear();
        {
            std::uint32_t all_slot{},extended_slot{};unsigned alive{};
            NativeCombatEventHandlers handlers(router,all_slot,extended_slot,
                [&](std::uint32_t,std::uint32_t entry,std::uint32_t payload){
                    if(entry!=0x57b520u)throw std::logic_error("unexpected SDK actor handler callback");
                    ++alive;manager.HandleBecomeAlive(*reinterpret_cast<const std::uint32_t*>(payload)-716u);
                });
            check(handlers.SubscriptionCount()==25u,"actual native handler owner subscribes17/8");
            for(std::size_t i=0;i<manager.Count();++i) {
                const auto a=manager.ActorAt(i);auto& model=manager.ModelOwner(a);
                const auto position=*reinterpret_cast<const std::array<float,3>*>(model.Model()->Address()+172u);
                model.PositionSceneModel({999.0f,999.0f,999.0f});
                const std::array payload{a+716u};
                (void)router.Dispatch("ae_become_alive",reinterpret_cast<std::uint32_t>(payload.data()));
                check(*reinterpret_cast<const std::array<float,3>*>(model.Model()->Address()+172u)==position,
                      "native57B520 restores actual formation position before motion");
            }
            check(alive==3u,"original three Monster alive events route through real handlers");
            handlers.Close();check(!all_slot && !extended_slot,"handler owner clears both publications");
            std::cout<<"Native actor handlers:17/8 subscriptions, three actual Monster formation/alive callbacks and retirement pass\n";
        }
        {
            // Captured M13/1 Monster2600 motion: identical position, one-ULP
            // modern-CRT yaw fork. Exercise the actual SDK/model route.
            const auto a=manager.ActorAt(0u);auto& motion=manager.ModelOwner(a);
            const auto child=motion.Model()->Address();
            const auto position=*reinterpret_cast<const ResourceVec3*>(child+172u);
            motion.PositionSceneModel({-34.f,0.f,-272.f});
            manager.UpdateMonsterMotion(a,0u);
            check(*reinterpret_cast<const std::uint32_t*>(child+164u)==0x3f869642u,
                  "M13 Monster formation yaw matches original legacy acos bits");
            motion.PositionSceneModel(position);manager.UpdateMonsterMotion(a,0u);
            std::cout<<"Native Monster motion acos: captured M13 position and real SDK/model yaw3F869642 pass\n";
        }
        VerifyNativeMonsterTargetSelection(database,manager,system,effects,router,combat);
        for(std::size_t i=0;i<manager.Count();++i) {
            const auto a=manager.ActorAt(i);
            auto& owner=manager.ModelOwner(a).Actor();
            const auto live=effects.CombatEffectCreation().Create("foot_2",{0,1,1},1,0u);
            auto& bottom=*reinterpret_cast<EffectSystemHandleWords*>(a+868u);
            bottom={live.id,live.system};
            check(EffectHandle::DialogNeedsUpdate(bottom,effects.ContentHandles()),"retirement starts with a real active effect");
            auto* state=reinterpret_cast<std::uint32_t*>(a+832u);
            *state=7u;
            bool rejected=false;*reinterpret_cast<std::uint32_t*>(a+848u)=1u;
            try{(void)manager.RetireActor(a,effects);}catch(const std::logic_error&){rejected=true;}
            check(rejected && *state==7u && manager.ModelOwner(a).Slot()==i && bottom.system==live.system,
                  "retirement rejects nonzero count with empty tree bounds before state/slot writes");
            *reinterpret_cast<std::uint32_t*>(a+848u)=0u;
            auto& conditional=manager.Skills(a);
            const auto prior_id=*reinterpret_cast<std::uint32_t*>(a+960u);
            const auto prior_rng=system.Random();
            if(i==0u){
                *reinterpret_cast<std::uint32_t*>(a+960u)=2600u;
                conditional.BindAllTargets([&](auto){std::vector<std::uint32_t> v;for(std::size_t n=0;n<manager.Count();++n)v.push_back(manager.ActorAt(n));return v;});
                conditional.BindConditionalRetirement([&]{
                    check(conditional.CurrentStat(7u)==0 && !conditional.ExitDispatchActive(),"9126 actual Monster HP reset precedes retirement without death permission");
                    (void)manager.RetireActorDuringSkill(a,effects);
                });
                bool denied=false;try{(void)manager.RetireActorDuringSkill(a,effects);}catch(const std::logic_error&){denied=true;}
                check(denied,"Monster skill retirement rejects calls outside9126");
            }
            const auto retire_actor=[&]()->std::uint32_t{
                if(i!=0u)return manager.RetireActor(a,effects);
                *state=2u;
                check(conditional.TriggerForTarget(7750u,a),"original7750 enters actual Monster retirement");
                return manager.ModelOwner(a).Slot();
            };
            bool fail_leave=i==0u;
            const auto observe=router.Subscribe(router.Key("me_leave_stage"),1u,[&](std::uint32_t payload){
                check(*reinterpret_cast<const std::uint32_t*>(payload)==a+716u && !*state && !bottom.system,
                      "retirement clears state832 before real leave event");
                bool nested=false;try{(void)manager.RetireActor(a,effects);}catch(const std::logic_error&){nested=true;}
                check(nested,"retirement pins manager during leave callback");
                nested=false;try{(void)RetireNativeCombatActor(owner,effects.ContentHandles(),[]{});}
                catch(const std::logic_error&){nested=true;}
                check(nested,"retirement pins actor during leave callback");
                if(fail_leave)throw std::logic_error("intentional retirement leave observer failure");
            });
            if(fail_leave) {
                bool failed=false;
                try{(void)retire_actor();}catch(const std::logic_error&){failed=true;}
                check(failed && !*state && !bottom.system && manager.ModelOwner(a).Slot()==i && manager.StageCount()==3u,
                      "failed leave retains prior effect/state writes without final slot reset");
                fail_leave=false;
            }
            try{check(retire_actor()==6u,"original retirement return6");}
            catch(...){(void)router.Unsubscribe(observe);throw;}
            (void)router.Unsubscribe(observe);
            if(i==0u){
                conditional.BindAllTargets({});conditional.BindConditionalRetirement({});
                *reinterpret_cast<std::uint32_t*>(a+960u)=prior_id;
                auto actual=system.Random(),reference=prior_rng;
                for(unsigned n=0;n<624u;++n)check(actual.Next()==reference.Next(),"actual9126 Monster retirement consumes no MT");
                std::cout<<"Native Combat9126 Monster: original7750 real effects/stage leave, throwing callback recovery, permit guard and no MT pass\n";
            }
            check(manager.ModelOwner(a).Slot()==6u && *reinterpret_cast<const std::uint32_t*>(a+332u)==6u,
                  "retirement resets mirrored slot after leave");
        }
        check(!manager.StageCount() && manager.Count()==3u,"retirement preserves first roster and clears stage through real events");
        std::cout<<"Native52DE40 Monster retirement: real3 leave events, empty-tree guard, state832 before leave, mirrored slot6 after leave and callback lifetime guards pass\n";
        manager.ClearActors(effects);manager.ClearActors(effects);
        check(!manager.Count() && !manager.StageCount() && manager.SubscriptionCount()==3u,"Monster clear retains reusable subscriptions");
        const auto again=manager.CreateMonster(2579,0u),again2=manager.CreateMonster(2580,1u);
        manager.InitializeMonster(again,0u,passive,effects);manager.InitializeMonster(again2,1u,passive,effects);
        unsigned first_leaves{},second_leaves{};bool fail_clear=true;
        const auto clear_observer=router.Subscribe(router.Key("me_leave_stage"),1u,[&](std::uint32_t payload){
            const auto a=*reinterpret_cast<const std::uint32_t*>(payload)-716u;
            check(manager.Count()==2u && manager.ModelOwner(again).Model() && manager.ModelOwner(again2).Model(),
                  "all Monster retirements precede release of any model");
            if(a==again)++first_leaves;else {check(a==again2,"clearing actual second Monster");++second_leaves;}
            bool blocked=false;try{manager.ClearActors(effects);}catch(const std::logic_error&){blocked=true;}
            check(blocked,"Monster clear reentry rejected");
            if(a==again2 && fail_clear)throw std::logic_error("intentional Monster clear leave failure");
        });
        try {
            bool failed=false;try{manager.ClearActors(effects);}catch(const std::logic_error&){failed=true;}
            check(failed && first_leaves==1u && second_leaves==1u && manager.StageCount()==1u,
                  "Monster failed second retirement preserves owners and completed first phase progress");
            failed=false;try{(void)manager.CreateMonster(2579,2u);}catch(const std::logic_error&){failed=true;}
            check(failed,"Monster creation blocked until clear retry succeeds");
            fail_clear=false;manager.ClearActors(effects);
        }catch(...){(void)router.Unsubscribe(clear_observer);throw;}
        (void)router.Unsubscribe(clear_observer);
        check(first_leaves==1u && second_leaves==2u && !manager.Count() && !manager.StageCount() && manager.SubscriptionCount()==3u,
              "Monster clear retry resumes current actor and preserves collection subscriptions");
        std::cout<<"Native Monster roster clear: real entry/effects, all retire before release, second-actor failure/retry, callback guards,3 retained subscriptions and recreation pass\n";
        unsigned creations{},joins{};bool fail_new{};
        observers.push_back(router.Subscribe(router.Key("me_new_added"),2u,[&](std::uint32_t payload){
            const auto before=manager.StageCount();
            const auto result=manager.HandleNewActor(payload,passive,effects);
            ++creations;
            if(before<6u){bool denied=false;try{(void)manager.HandleNewActor(payload,passive,effects);}
                catch(const std::logic_error&){denied=true;}check(denied,"same creation cannot initialize twice");}
            check(result==(before<6u?1u:6u),"original new-Monster callback reports initialized or full formation");
            if(fail_new)throw std::logic_error("intentional new-Monster callback failure");
        }));
        observers.push_back(router.Subscribe(router.Key("me_join_stage"),1u,[&](std::uint32_t payload){
            bool denied=false;try{(void)manager.HandleNewActor(payload,passive,effects);}catch(const std::logic_error&){denied=true;}
            check(denied,"new-Monster callback rejects recursive initialization");
            const auto a=*reinterpret_cast<const std::uint32_t*>(payload)-716u;
            denied=false;try{manager.Skills(a).NotifyNewActor();}catch(const std::logic_error&){denied=true;}
            check(denied,"new-actor skill notification rejects recursion");
            denied=false;try{manager.Skills(a).CleanupObjectLists(passive);}catch(const std::logic_error&){denied=true;}
            check(denied,"deeper initialization callback cannot reenter passive cleanup");
            denied=false;try{(void)manager.CreateMonster(2580,0u);}catch(const std::logic_error&){denied=true;}
            check(denied,"synchronous initialization pins creation owner");++joins;
        }));
        const std::array<unsigned,7> requested{5u,5u,99u,6u,6u,6u,6u};
        const std::array<unsigned,7> selected{5u,0u,1u,2u,3u,4u,6u};
        for(unsigned i=0;i<requested.size();++i){
            const auto a=manager.CreateMonster(i?2580:2579,requested[i]);
            check(manager.ModelOwner(a).Slot()==selected[i] && manager.StageCount()==std::min(i+1u,6u),
                  "real me_new_added selects requested/free/fallback/full formation during CreateMonster");
            check(bool(manager.ModelOwner(a).Model())==(i<6u),"full formation retains offstage owner without inventing a model");
            if(i<6u)check(manager.ModelOwner(a).Model()->Asset()->CurrentAction()=="Z01" &&
                EffectHandle::DialogNeedsUpdate(*reinterpret_cast<const EffectSystemHandleWords*>(a+868u),effects.ContentHandles()),
                "new-Monster callback completes actual52CFD0 model/action/bottom effect before factory returns");
            const std::array payload{a+716u};bool denied=false;
            try{(void)manager.HandleNewActor(reinterpret_cast<std::uint32_t>(payload.data()),passive,effects);}
            catch(const std::logic_error&){denied=true;}check(denied,"stale new-actor event cannot repeat initialization");
        }
        check(creations==7u && joins==6u && !manager.ActiveIterators(),"six actual stage entries and full-slot rejection");
        drain();manager.ClearActors(effects);
        fail_new=true;bool failed=false;
        try{(void)manager.CreateMonster(2579,0u);}catch(const std::logic_error&){failed=true;}
        check(failed && manager.Count()==1u && manager.StageCount()==1u && manager.ModelOwner(manager.ActorAt(0u)).Model(),
              "throwing new callback retains inserted initialized actor and earlier writes");
        fail_new=false;
        const auto after_failure=manager.CreateMonster(2580,0u);
        check(manager.ModelOwner(after_failure).Slot()==1u && manager.StageCount()==2u,
              "new-event dispatch guard recovers and next actor avoids occupied slot after exception");
        for(const auto h:observers)(void)router.Unsubscribe(h);observers.clear();
        drain();manager.ClearActors(effects);
        std::cout<<"Native57B1A0 new Monster: actual creation callback, requested/duplicate/fallback/full6 slots, synchronous52CFD0/effects, recursion and failure recovery pass\n";
        for(const auto id:{2583,2584}) {
            const auto boss=manager.CreateMonster(id,1u);
            check(boss!=0u,"M07 boss exists in original database");
            manager.InitializeMonster(boss,1u,passive,effects);
            check(manager.StageCount()==1u && manager.ModelOwner(boss).Model(),
                  "M07 boss completes real entry and effect initialization");
            effects.UpdateCombatEffects(0.04);
            manager.ClearActors(effects);
            check(!manager.Count() && !manager.StageCount(),"M07 boss entry retires before next battle");
        }
        std::cout<<"Native52CFD0: original M07 bosses2583/2584 sequential effect entry and retirement pass\n";
        manager.Close();pool.Clear();system.Destroy();
        check(!system.OwnedAllocations(),"entry dependencies retire");
    } catch(...) {for(const auto h:observers)(void)router.Unsubscribe(h);drain();throw;}
    std::cout<<"Native52CFD0: original cave2579/2580/2580 full shared entry verified\n";
}
}
