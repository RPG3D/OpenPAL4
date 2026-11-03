#pragma once
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_actor_instance_pool.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_actor_turn_services.h"
#include "pal4/actor_animation_control.h"
#include "pal4/native_combat_actor_action_tasks.h"
#include "pal4/native_combat_fatal_children.h"
#include "pal4/combat_script_task_operations.h"
#include "pal4/combat_status_tasks.h"
#include "pal4/gom_task_base.h"
#include "pal4/gom_task_container.h"
#include "pal4/native_rw_geometry_processor.h"
#include "pal4/rw_plugin_8449_native.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/native_player_collection_values.h"
#include "pal4/native_script_event_router.h"
#include "pal4/random_number_generator.h"
#include <vector>
#include <array>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <rpworld.h>
#include <rphanim.h>
#include <rtanim.h>

namespace pal4::test {
inline void VerifyNativeCombatActorModel(const std::filesystem::path& root) {
    auto check=[](bool ok,const char* m) { if (!ok) throw std::runtime_error(m); };
    auto word=[](std::uint32_t a)->std::uint32_t& { return *reinterpret_cast<std::uint32_t*>(a); };
    std::array<std::uint32_t,128> game{};
    const auto game_address=reinterpret_cast<std::uint32_t>(game.data());
    NativeGameDatabase db;
    check(db.Load(root),"combat binding original Monster database");
    NativeMonsterRecordCatalog records(db);
    bool inactive=true;
    unsigned loads=0u;
    unsigned fault=0u;
    std::uint32_t fault_address{},fault_original{};
    NativeActorInstancePool pool([&] { return inactive; },[&](std::string_view name) {
        ++loads;
        auto model=std::make_unique<NativeActorInstanceOwner>();
        model->Construct(game_address); model->Load(root,name);
        if (fault) {
            fault_address=fault==1u ? word(model->Address()+376u)-4u : model->Address()+492u;
            fault_original=word(fault_address);
            word(fault_address)=fault==1u ? fault_original-1u : game_address;
        }
        return model;
    });
    NativeCombatActorModelOwner first(pool),second(pool);
    const auto a=first.Construct(records,2579);
    const auto b=second.Construct(records,2580);
    check(first.Attach("B01")==0u && !first.Model() && !word(a+808u) && !loads,
          "inactive Combat does not attach or load a model");
    inactive=false;
    check(first.Attach("B01")==a && first.Model(),"native attach returns actual monster parent");
    const auto child=first.Model()->Address();
    check(word(a+808u)==child && word(child+492u)==a && word(child+516u)==a &&
          first.Model()->Asset()->CurrentAction()=="Z01",
          "actual actor808, child parents and SDK default action agree");
    constexpr std::array<std::array<float,3>,6> positions{{
        {-92.0f,0.0f,-102.0f},{41.0f,0.0f,-148.0f},{161.0f,0.0f,-91.0f},
        {-159.0f,0.0f,-230.0f},{-34.0f,0.0f,-272.0f},{126.0f,0.0f,-218.0f}}};
    for (std::uint32_t slot=0;slot<positions.size();++slot) {
        first.SetSlot(slot);
        check(first.Slot()==slot && word(a+332u)==slot && word(a+668u)==slot,
              "native property setter reuses mirrored base/derived slot writes");
        check(first.RefreshResource()!=0u && first.Model()->Address()==child && loads==1u,
              "531690 refresh uses actual record model and reuses the same SDK owner");
        const auto* ltm=RwFrameGetLTM(RpClumpGetFrame(first.Model()->Asset()->Main()));
        const auto& expected=positions[slot];
        check(std::fabs(ltm->pos.x-expected[0])<0.001f &&
              std::fabs(ltm->pos.y-expected[1])<0.001f &&
              std::fabs(ltm->pos.z-expected[2])<0.001f &&
              *reinterpret_cast<float*>(child+172u)==expected[0] &&
              *reinterpret_cast<float*>(child+180u)==expected[2],
              "six original monster slot descriptors reach stored and actual SDK clump position");
    }
    first.SetSlot(6u);
    (void)first.RefreshResource();
    check(word(child+172u)==0xffffffffu && word(child+176u)==0xffffffffu &&
          word(child+180u)==0xffffffffu,
          "out-of-range slot preserves original default NaN descriptor without clamping");
    first.SetSlot(0u); (void)first.RefreshResource();
    check(std::isfinite(RwFrameGetLTM(RpClumpGetFrame(first.Model()->Asset()->Main()))->pos.x),
          "valid slot refresh restores SDK transform after default descriptor");
    check(first.Attach("B02")==a && pool.CachedCount()==1u && loads==2u,
          "replacement returns previous child before creating the next");
    check(second.Attach("B01")==b && second.Model()->Address()==child && loads==2u &&
          word(child+492u)==b && word(child+516u)==b,
          "second actual monster reuses cached child with new parent links");
    const auto current=first.Model()->Address();
    word(current+624u+408u)=game_address;
    bool rejected=false;
    try { first.Detach(); } catch(const std::logic_error&) { rejected=true; }
    check(rejected && first.Model()->Address()==current && word(a+808u)==current &&
          pool.CachedCount()==0u,"failed detach retains actor808 and owning model");
    word(current+624u+408u)=0u;
    first.Detach(); first.Detach();
    check(!first.Model() && !word(a+808u) && word(current+492u)==a && pool.CachedCount()==1u,
          "successful detach clears actor808 only after cache owns the child");
    rejected=false;
    try { (void)first.Attach("missing_combat_actor"); } catch(const std::runtime_error&) { rejected=true; }
    check(rejected && !first.Model() && !word(a+808u) && pool.CachedCount()==1u,
          "loader failure leaves no published child and preserves cache");
    pool.Clear();
    fault=1u; rejected=false;
    try { (void)first.Attach("B02"); } catch(const std::logic_error&) { rejected=true; }
    check(rejected && !first.Model() && !word(a+808u) && pool.CachedCount()==1u,
          "frame failure returns the new owner and clears actor808 during rollback");
    word(fault_address)=fault_original; fault=0u; pool.Clear();
    fault=2u; rejected=false;
    try { (void)first.Attach("B02"); } catch(const std::logic_error&) { rejected=true; }
    check(rejected && first.Model() && word(a+808u)==first.Model()->Address() &&
          pool.CachedCount()==0u,"rollback failure retains a recoverable matching owner and actor808");
    word(fault_address)=fault_original; fault=0u;
    first.Detach();
    first.Destroy(); first.Destroy(); second.Destroy();
    check(!first.Address() && !second.Address() && !first.Model() && !second.Model() &&
          pool.CachedCount()==2u,"actor destruction returns models before original constructor owners retire");
    pool.Clear();
    {
        NativeCombatActorModelOwner joined(pool),foreign(pool);
        const auto actor=joined.Construct(records,2579);
        (void)foreign.Construct(records,2580);
        NativeScriptEventRouter router;
        NativeSkillRecordCatalog catalog(db);
        RandomNumberGenerator random;
        NativePlayerCombatEvents events;
        NativeSkillSystemOwner system(random,events);
        (void)system.Construct("H:/PAL4/objs");
        NativeCombatActorSkillRuntime skills(joined.Actor(),router,catalog,system);
        (void)skills.RebuildStats();
        joined.SetSlot(2u);
        const auto hp=word(actor+344u),mp=word(actor+352u);
        check(hp>0u,"original cave monster has max HP");
        auto lower=[&] { word(actor+572u)=1u; word(actor+580u)=0u; };
        lower();
        bool mismatch=false;
        try { skills.JoinStageAndScene(foreign); } catch(const std::invalid_argument&) { mismatch=true; }
        check(mismatch && word(actor+572u)==1u && !joined.Model(),
              "foreign model rejected before HP/model mutations");
        std::vector<unsigned> order;
        bool fail_stage=false,expect_model=true;
        const auto stage=router.Subscribe(router.Key("me_join_stage"),1u,[&](std::uint32_t payload) {
            check(word(payload)==actor+716u && word(actor+572u)==hp && word(actor+580u)==mp,
                  "join stage receives actual actor facet after HP/MP restoration");
            check(bool(joined.Model())==expect_model,"model refresh precedes stage event");
            if(expect_model) {
                const auto* matrix=RwFrameGetLTM(RpClumpGetFrame(joined.Model()->Asset()->Main()));
                check(std::fabs(matrix->pos.x-161.0f)<0.001f && std::fabs(matrix->pos.z+91.0f)<0.001f,
                      "join positions actual SDK clump using original slot2 descriptor");
            }
            bool close_rejected=false,destroy_rejected=false;
            try { skills.Close(); } catch(const std::logic_error&) { close_rejected=true; }
            try { joined.Actor().Destroy(); } catch(const std::logic_error&) { destroy_rejected=true; }
            check(close_rejected && destroy_rejected,"synchronous join guards actor/runtime retirement");
            order.push_back(1u);
            if(fail_stage) throw std::runtime_error("join observer failure");
        });
        const auto scene=router.Subscribe(router.Key("me_join_scene"),1u,[&](std::uint32_t payload) {
            check(word(payload)==actor+716u,"join scene actual facet payload"); order.push_back(2u);
        });
        //531720 intentionally ignores zero from inactive531690.
        inactive=true; expect_model=false;
        skills.JoinStageAndScene(joined);
        check(order==std::vector<unsigned>{1u,2u},"inactive refresh still emits original join sequence");
        inactive=false; expect_model=true; order.clear(); lower();
        skills.JoinStageAndScene(joined);
        check(order==std::vector<unsigned>{1u,2u} && word(actor+808u)==joined.Model()->Address(),
              "active join attaches real B01 before ordered stage and scene events");
        order.clear(); lower(); fail_stage=true;
        bool failed=false;
        try { skills.JoinStageAndScene(joined); } catch(const std::runtime_error&) { failed=true; }
        check(failed && order==std::vector<unsigned>{1u} && word(actor+572u)==hp && joined.Model(),
              "subscriber failure retains preceding health/model effects and suppresses scene event");
        order.clear(); fail_stage=false; skills.JoinStageAndScene(joined);
        check(order==std::vector<unsigned>{1u,2u},"join retries after observer repair");
        {
            class TurnModel final : public INativeCombatActorTurnServices {
            public:
                NativeCombatActorModelOwner& model;
                std::array<unsigned,4u> queue{};
                explicit TurnModel(NativeCombatActorModelOwner& m):model(m) {}
                [[noreturn]] static void Unbound(){throw std::logic_error("Combat death/scene owner not bound");}
                unsigned CreateDeathAction(unsigned) override {Unbound();}
                void RegisterActorAction(unsigned) override {Unbound();}
                void ApplyPendingSceneRecord(unsigned) override {Unbound();}
                unsigned GeometryQueue() override {return reinterpret_cast<unsigned>(queue.data());}
                IActorAnimationControlRuntime& AnimationRuntime() override {return model.AnimationRuntime();}
            } turn(joined);
            const auto clump=joined.Model()->Asset()->Main();
            std::vector<RpAtomic*> atomics;
            RpClumpForAllAtomics(clump,[](RpAtomic* atomic,void* user){
                static_cast<std::vector<RpAtomic*>*>(user)->push_back(atomic);return atomic;
            },&atomics);
            check(!atomics.empty(),"original B01 alpha probe has SDK atomics");
            auto verify_alpha=[&](unsigned expected) {
                unsigned materials{};
                for(auto* atomic:atomics) {
                    auto* geometry=RpAtomicGetGeometry(atomic);
                    for(int i=0;i<RpGeometryGetNumMaterials(geometry);++i) {
                        const auto* color=RpMaterialGetColor(RpGeometryGetMaterial(geometry,i));
                        check(color->red==255u && color->green==255u && color->blue==255u && color->alpha==expected,
                              "native turn alpha writes actual original SDK material RGBA");
                        ++materials;
                    }
                }
                check(materials>0u,"original B01 alpha probe has materials");
            };
            unsigned turn_events{};
            const auto turn_observer=router.Subscribe(router.Key("me_can_take_action"),1u,[&](unsigned p){
                check(word(p)==actor+716u,"modeled turn emits actual facet");
                verify_alpha(word(actor+640u)==4u?12u:255u);
                ++turn_events;
            });
            word(actor+640u)=4u;skills.CanTakeAction(turn);
            verify_alpha(12u);
            const auto plugin_offset=NativeRwPlugin8449Lifecycle::State().geometry_offset;
            check(plugin_offset>0,"actual8449 plugin is registered");
            for(auto* atomic:atomics) {
                const auto address=reinterpret_cast<unsigned>(atomic);
                check(word(address+plugin_offset)!=0u && word(address+plugin_offset+4u)==1u &&
                      word(address+plugin_offset+8u)==turn.GeometryQueue() &&
                      reinterpret_cast<unsigned>(RpAtomicGetRenderCallBack(atomic))==NativeRwGeometryProcessor::RenderCallbackAddress(),
                      "alpha classification binds actual SDK atomic to its transparent queue and original callback");
            }
            check(word(joined.Model()->Address()+520u)==0xffffffffu,
                  "transient alpha preserves original cached frame color520");
            word(actor+640u)=0u;skills.CanTakeAction(turn);
            verify_alpha(255u);
            check(turn_events==2u,"model materials update before both turn notifications");
            // Death544 resets special status through the original156/property256
            // path, which must restore actual SDK material alpha before its event.
            NativePlayerCollectionCatalog passives(db);
            word(actor+640u)=4u;skills.CanTakeAction(turn);verify_alpha(12u);
            (void)skills.SetCurrentStat(7u,0);
            unsigned dead_events{};
            const auto death_observer=router.Subscribe(router.Key("ae_become_dead"),1u,[&](unsigned p){
                check(word(p)==actor+716u && word(p+4u)==b && word(actor+640u)==0u &&
                      word(actor+648u)==3u && word(actor+828u)==4u,
                      "modeled death544 emits exact facet/cause after real reset/rebuild");
                verify_alpha(255u);++dead_events;
            });
            skills.BecomeDead(b,passives,&turn);
            check(dead_events==1u,"original B01 modeled death reaches actual native lifecycle");
            check(router.Unsubscribe(death_observer),"modeled death observer cleanup");
            (void)skills.SetCurrentStat(7u,static_cast<int>(hp));
            check(word(actor+828u)==1u && word(actor+648u)!=3u,"health revival restores modeled actor before later motion tests");
            // Original processing retains registered callbacks when material
            // alpha becomes opaque. Restore explicitly before the queue dies.
            NativeRwGeometryProcessor::RestoreClump(clump);
            for(auto* atomic:atomics)
                check(word(reinterpret_cast<unsigned>(atomic)+plugin_offset+4u)==0u &&
                      reinterpret_cast<unsigned>(RpAtomicGetRenderCallBack(atomic))==
                          word(reinterpret_cast<unsigned>(atomic)+plugin_offset),
                      "alpha queue callback retired before borrowed queue lifetime ends");
            check(router.Unsubscribe(turn_observer),"modeled turn observer cleanup");
            std::cout<<"Original B01 turn alpha: SDK materials12/255, actual8449 queue payload/callback and cleanup pass\n";
            std::cout<<"Original B01 death544: status reset restores SDK alpha255 before death event, stats/health/revival pass\n";
        }
        {
            class UnusedDeath final : public ICombatScriptTaskRuntime,public ICombatStatusTaskRuntime {
            public:
                [[noreturn]] static void Unbound(){throw std::logic_error("unexpected death operation in ChangeAction probe");}
                unsigned ReadWord(unsigned a) override{return *reinterpret_cast<unsigned*>(a);}
                std::uint8_t ReadByte(unsigned a) override{return *reinterpret_cast<std::uint8_t*>(a);}
                void WriteWord(unsigned a,unsigned v) override{*reinterpret_cast<unsigned*>(a)=v;}
                void FireEvent(unsigned,unsigned,unsigned) override{Unbound();}
                void CallActor(unsigned,unsigned,unsigned) override{Unbound();}
                void SetProtectTarget(unsigned,unsigned,unsigned) override{Unbound();}
                void ConstructBase(unsigned) override{Unbound();}
                void SetStateOne(unsigned) override{Unbound();}
            } unused;
            NativeCombatActorActionTasks actions(joined,system);
            NativeCombatFatalChildren children(unused,unused,actions);
            GomTaskBase base(children.BaseRuntime());
            auto& runtime=children.ContainerRuntime();GomTaskContainer queue(runtime,base);
            GomTaskContainerState state{};
            const auto context=reinterpret_cast<unsigned>(&state);
            queue.Initialize(state,0);
            check(word(actor+804u)==2u,"original Monster requires death effect/fade pair");
            for(std::size_t i=0;i<joined.Model()->Asset()->ActionCount();++i)
                std::cout<<"B01 collapse action="<<joined.Model()->Asset()->ActionAt(i)<<" resource="<<joined.Model()->Asset()->ActionResourceAt(i)<<'\n';
            joined.PlayAction("z01",1u,system);
            auto* hierarchy=joined.Model()->Asset()->Hierarchy();
            auto* initial_clip=hierarchy->currentAnim->pCurrentAnim;
            check(RtAnimInterpolatorSetCurrentTime(hierarchy->currentAnim,0.1f)!=0,"advance original B01 before collapse");
            const auto task=children.Create(0x845d34u);
            children.AssignAction(task,"z02");word(task+60u)=foreign.Address();
            (void)queue.AddTaskAndInitialize(state,context,runtime.TaskAt(task),task,"ChangeActionTask",{1u,0,0});
            (void)queue.BeginWaiting(state);
            bool wrong_actor=false;
            try{(void)queue.Update(state,0.1f);}catch(const std::invalid_argument&){wrong_actor=true;}
            check(wrong_actor && runtime.TaskAt(task).phase==2u && hierarchy->currentAnim->pCurrentAnim==initial_clip,
                  "foreign ChangeAction receiver rejects before SDK mutation and remains retryable");
            word(task+60u)=actor;(void)queue.Update(state,0.1f);
            check(_stricmp(std::string(joined.Model()->Asset()->CurrentAction()).c_str(),"z02")==0 &&
                  hierarchy->currentAnim->pCurrentAnim!=initial_clip && hierarchy->currentAnim->currentTime==0.0f &&
                  word(joined.Model()->Address()+436u)==0u,
                  "scheduled native actor564 selects actual B01 Z02 clip and forwards restart0 to SDK");
            for(unsigned tick=0;tick<8u && !state.ready;++tick)(void)queue.Update(state,0.1f);
            check(state.ready && !children.Count() && !children.OwnedActionTexts(),"actual collapse task retires without animation polling");
            const auto missing=children.Create(0x845d34u);
            children.AssignAction(missing,"z12");word(missing+60u)=actor;
            auto* retained=hierarchy->currentAnim->pCurrentAnim;
            check(RtAnimInterpolatorSetCurrentTime(hierarchy->currentAnim,0.1f)!=0,"advance B01 before missing action");
            (void)queue.AddTaskAndInitialize(state,context,runtime.TaskAt(missing),missing,"ChangeActionTask",{1u,0,0});
            (void)queue.BeginWaiting(state);
            for(unsigned tick=0;tick<8u && !state.ready;++tick)(void)queue.Update(state,0.1f);
            check(state.ready && hierarchy->currentAnim->pCurrentAnim==retained &&
                  hierarchy->currentAnim->currentTime==0.1f &&
                  _stricmp(std::string(joined.Model()->Asset()->CurrentAction()).c_str(),"z02")==0,
                  "original missing Z12 preserves current B01 clip/time without aborting death task");
            // Original actor564 ignores text and system when actor808 is null.
            NativeCombatActorActionTasks detached(foreign,system);
            detached.SetActorTarget(foreign.Address(),0u,0u);
            (void)queue.Destroy(state);
            check(!children.OwnedAllocations(),"actual collapse GOM storage balanced");
            joined.PlayAction("z01",1u,system);
            std::cout<<"Original B01 ChangeAction: child/GOM -> actor564 -> SDK Z02, missing Z12 retains clip/time, restart0, retry and cleanup pass\n";
        }
        bool wrong_motion=false;
        try { skills.UpdateMotion(foreign); } catch(const std::invalid_argument&) { wrong_motion=true; }
        check(wrong_motion,"motion rejects foreign model before mutation");
        const auto motion_child=joined.Model()->Address();
        for(unsigned slot=0u;slot<6u;++slot) {
            joined.SetSlot(slot);
            auto* hierarchy=joined.Model()->Asset()->Hierarchy();
            check(RtAnimInterpolatorSetCurrentTime(hierarchy->currentAnim,0.1f)!=0,
                  "SDK animation time advanced before motion restart");
            skills.UpdateMotion(joined);
            const auto* matrix=RwFrameGetLTM(RpClumpGetFrame(joined.Model()->Asset()->Main()));
            const auto& pos=positions[slot];
            const double norm=std::hypot(-33.0,304.0);
            const double dx=8.0-33.0/norm*300.0-pos[0];
            const double dz=156.0+304.0/norm*300.0-pos[2];
            double angle=std::atan2(dx,dz)*180.0/3.141592653589793;
            if(angle<0.0) angle+=360.0;
            std::cout << "Motion slot=" << slot << " actor_state=" << word(actor+828u)
                      << " health_state=" << word(actor+648u) << " position=" << matrix->pos.x << "," << matrix->pos.z
                      << " yaw=" << *reinterpret_cast<float*>(motion_child+164u) << " expected=" << angle << "\n";
            // The original SDK normalizer is approximate; acos amplifies its
            // error near the forward axis (slot4 is about0.3 degrees apart).
            // Keep independent geometric bounds, plus actual SDK frame checks.
            check(std::fabs(matrix->pos.x-pos[0])<0.001f && std::fabs(matrix->pos.z-pos[2])<0.001f &&
                  std::fabs(*reinterpret_cast<float*>(motion_child+164u)-angle)<0.5,
                  "six original slots reposition and store native signed heading normalized by shared transform");
            check(std::fabs(matrix->at.x-dx/std::hypot(dx,dz))<0.01 &&
                  std::fabs(matrix->at.z-dz/std::hypot(dx,dz))<0.01,
                  "actual SDK clump forward vector faces original battlefield destination");
            check(joined.Model()->Asset()->CurrentAction()=="Z01" &&
                  word(motion_child+368u+68u)==1u && hierarchy->currentAnim->currentTime==0.0f &&
                  *reinterpret_cast<std::uint8_t*>(motion_child+624u+460u)==1u &&
                  *reinterpret_cast<std::uint8_t*>(motion_child+1088u+460u)==1u,
                  "motion runs actual action/empty hand activation, restart flag and SDK animation reset");
        }
        // Only the low byte controls reposition; heading and action still update.
        joined.SetSlot(0u); skills.UpdateMotion(joined,0x100u);
        check(std::fabs(*reinterpret_cast<float*>(motion_child+172u)-positions[5][0])<0.001f,
              "flag256 leaves position as original low-byte test requires");
        skills.UpdateMotion(joined,1u);
        for(const auto state:{0u,3u,4u,6u,7u}) {
            word(actor+828u)=state; joined.SetSlot(1u); skills.UpdateMotion(joined);
            check(std::fabs(*reinterpret_cast<float*>(motion_child+172u)-positions[0][0])<0.001f,
                  "unready actor state suppresses motion");
        }
        for(const auto state:{1u,2u,5u}) {
            word(actor+828u)=state; joined.SetSlot(state%6u); skills.UpdateMotion(joined);
            check(std::fabs(*reinterpret_cast<float*>(motion_child+172u)-positions[state%6u][0])<0.001f,
                  "ready states1/2/5 permit actual motion");
        }
        word(actor+648u)=3u; joined.SetSlot(0u); skills.UpdateMotion(joined);
        check(std::fabs(*reinterpret_cast<float*>(motion_child+172u)-positions[5][0])<0.001f,
              "actual state-three property blocks motion despite ready actor flag");
        word(actor+648u)=0u;
        *reinterpret_cast<std::uint8_t*>(actor+1160u)=1u;
        bool roster_missing=false;
        try { skills.UpdateMotion(joined); } catch(const std::logic_error& e) {
            roster_missing=std::string_view(e.what()).find("roster iterator")!=std::string_view::npos;
        }
        check(roster_missing && !*reinterpret_cast<std::uint8_t*>(actor+1160u),
              "missing original Monster roster rejects after original1160 clear; no fabricated iterator");
        skills.UpdateMotion(joined);
        std::cout << "Original5317E0 six-slot SDK position/heading/Z01/hand state, readiness and explicit roster boundary verified\n";
        check(router.Unsubscribe(stage) && router.Unsubscribe(scene),"join observers unsubscribe");
        skills.Close(); joined.Destroy(); foreign.Destroy(); pool.Clear(); system.Destroy();
        check(!joined.Actor().OwnedAllocations() && !foreign.Actor().OwnedAllocations() &&
              !system.OwnedAllocations() && !pool.CachedCount(),"joined actor/model/skill/system retirement");
        std::cout << "Original531720 native Monster HP/MP, SDK model/slot and ordered join events verified\n";
    }
    std::cout << "Combat actual actor808 attach/replace/reuse/detach/rollback verified\n";
}
} // namespace pal4::test
