#pragma once
#include "pal4/native_combat_player_scene_actors.h"
#include "pal4/native_combat_player_motion.h"
#include "pal4/native_combat_actor_retirement.h"
#include "pal4/native_combat_player_model_cache.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_actor_manager_owner.h"
#include "native_combat_player_modeled_death_probe.h"
#include "native_combat_player_entry_probe.h"
#include "native_combat_player_new_actor_probe.h"
#include "native_combat_action_lifecycle_probe.h"
#include "native_combat_first_roster_probe.h"
#include "native_combat_protection_children_probe.h"
#include "native_combat_action_wait_probe.h"
#include "native_combat_reaction_children_probe.h"
#include "native_combat_miss_actions_probe.h"
#include "native_combat_fend_off_impact_probe.h"
#include "native_combat_message_children_probe.h"
#include "native_combat_reaction_composition_probe.h"
#include "native_combat_attack_actions_probe.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_actor_instance_pool.h"
#include "pal4/native_actor_instance_owner.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_script_event_router.h"
#include <array>
#include <cstring>
#include <vector>
#include <stdexcept>
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_combat_world_resources.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_world_scene_events.h"
#include "pal4/world_manager_event_subscriptions.h"
#include "pal4/native_combat_world_frame_services.h"
#include "pal4/native_combat_prerequisites.h"
#include "pal4/native_ui_world_camera.h"
#include "pal4/native_world_asset.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_camera_manager_owner.h"
#include <d3d9.h>
#include <algorithm>
#include <iostream>
namespace pal4::test {
inline void VerifyNativeCombatPlayerWorld(NativePlayerManagerSession& source,
    const NativeGameDatabase& database,NativePkFileManagerRuntime& packages,
    const std::filesystem::path& root,NativePalGameIVTextureStorage& game,
    NativeCameraManagerOwner& cameras,INativeSkillSystemServices& skills,NativeEffectSceneSession& effects) {
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto addr=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    const auto reject=[&](auto&& f){bool denied=false;try{f();}catch(const std::logic_error&){denied=true;}check(denied,"Player world expected rejection");};
    const auto active=cameras.ActiveCameraPointer();
    NativeInventorySession inventory;inventory.BindDatabase(database);(void)inventory.Start();
    const auto archives=packages.SceneArchiveCount();
    NativeCombatSystemImage storage;
    storage.target_selection.words.front()=0x13579bdfu;
    storage.world_text_storage.back()=0x2468ace0u;
    storage.session_storage.front()=0x10203040u;
    NativeCombatWorldResources world(packages,game,storage.world);world.Load(root,"CombatWorld","M01");
    NativeCombatTranslator translator(storage.translator);
    check(translator.Load(root) && translator.Table(),"embedded Combat translator loads original database");
    check(world.Address()==addr(&storage)+116u && translator.Address()==addr(&storage)+380u &&
          *reinterpret_cast<const std::uint8_t*>(addr(&storage)+517u)==0u,
          "actual world and translator occupy original CombatSystem subobjects without cold-init");
    auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    NativeMatFxScriptSession shader(packages,device);check(shader.Load(),"Player world original MatFX");world.InstallRenderPipeline(shader.Effect());
    NativeScriptEventRouter router;NativeCombatActorManagerImage manager_image;
    NativeCombatManagerEvents::ResolveActor resolve;
    // Bind before entry callbacks create target-bearing Dialogs. Reuse the
    // same live actor/model resolver as the actual ActorManager events.
    effects.BindDialogFacetReader([&](std::uint32_t facet){
        if(facet<724u || !resolve)throw std::invalid_argument("unbound or foreign Combat position facet");
        const auto view=resolve(facet-724u);
        if(!view.position)throw std::logic_error("Combat facet has no actual model position");
        std::array<float,3> value{};std::memcpy(value.data(),reinterpret_cast<const void*>(view.position),sizeof(value));return value;
    });
    struct FacetBinding {NativeEffectSceneSession& e;~FacetBinding(){try{e.BindDialogFacetReader({});}catch(...){}}} facet_binding{effects};
    NativeCombatActorManagerOwner manager_owner(manager_image,storage,effects,router,
        [&](std::uint32_t actor){return resolve(actor);});
    check(manager_owner.SubscriptionCount()==10u && storage.ActorManagerSlot()==manager_owner.Address(),
        "actual ActorManager owner publishes member424 and ten original handlers");
    NativeCombatActorManagerImage duplicate_image;
    reject([&]{NativeCombatActorManagerOwner duplicate(duplicate_image,storage,effects,router,
        [](std::uint32_t){return NativeCombatEventActor{};});});
    struct Effects {NativeEffectSceneSession& e;bool active=true;~Effects(){if(active)try{e.ClearCombatEffects();e.UnbindCombatEffects();}catch(...){}}} retire{effects};
    NativeActorInstancePool pool([]{return false;},[&](std::string_view name){
        auto model=std::make_unique<NativeActorInstanceOwner>();(void)model->Construct(game.Address());model->Load(root,name);
        model->BindHandResources(root,packages,effects,skills,[]{return false;});return model;
    });
    NativeMonsterRecordCatalog monsters(database);NativeSkillRecordCatalog skill_records(database);
    NativeCombatSkillActorRegistry registry(router,skill_records,skills);
    NativeCombatMonsterManager monster_actors(pool,monsters,skill_records,router,skills,registry);
    NativeCombatPlayerManager players(source,router,skills);
    const auto first=players.AddPlayer(0u),second=players.AddPlayer(1u),absent=players.AddPlayer(2u);
    players.Actor(first).SetSlot(0u);players.Actor(second).SetSlot(1u);
    NativeCombatPlayerModelCache one(players.Actor(first),pool,inventory,120300),
        two(players.Actor(second),pool,inventory,120300),empty(players.Actor(absent),pool,inventory,120300);
    NativeCombatPlayerSceneActors actors(players,router);actors.Register(one);actors.Register(two);actors.Register(empty);
    resolve=[&](std::uint32_t a){
        for(auto* cache:{&one,&two,&empty})if(cache->ActorAddress()==a){
            const auto child=cache->Current().Address();
            return NativeCombatEventActor{1u,child+172u,child+160u,word(a+1192u)};
        }
        for(std::size_t i=0;i<monster_actors.Count();++i)if(monster_actors.ActorAt(i)==a){
            const auto* child=monster_actors.ModelOwner(a).Model();
            if(!child)throw std::logic_error("ActorManager Monster model is not prepared");
            return NativeCombatEventActor{2u,child->Address()+172u,child->Address()+160u,std::nullopt};
        }
        throw std::invalid_argument("foreign ActorManager test actor");
    };
    std::array<std::uint32_t,8> foreign_slots{};
    reject([&]{NativeCombatWorldSceneEvents invalid(world,router,monster_actors,&actors,&foreign_slots);});
    NativeCombatWorldSceneEvents scene(world,router,monster_actors,&actors,&storage.world_event_storage);
    check(scene.SubscriptionCount()==4u,"actual WorldManager84 publishes four scene subscriptions");
    for(unsigned i=0;i<4u;++i)check(router.Contains(storage.world_event_storage[2u*i]) &&
        storage.world_event_storage[2u*i+1u]==router.Key(WorldManagerEventSubscriptions::kBindings[i].name),
        "WorldManager84 actual handle/key publication");
    reject([&]{one.Close();});reject([&]{players.Close();});reject([&]{actors.Close();});
    NativeUiWorldCamera camera;check(camera.Open(world.Asset().World(),320u,240u),"Player M01 offscreen camera");
    (void)SetupNativeCombatCamera(cameras);
    const auto& data=cameras.CameraDataForObject(cameras.FindByName("CombatCamera"));
    auto* original=reinterpret_cast<RwCamera*>(data.At(0u));
    manager_owner.Initialize(game.Address(),addr(original));
    check(effects.CombatScreenEffectHandlerCount()==15u,"actual ActorManager initializes native game-object handlers");
    check(RwFrameTransform(RwCameraGetFrame(camera.Camera()),RwFrameGetLTM(RwCameraGetFrame(original)),rwCOMBINEREPLACE)!=nullptr,"Player M01 original camera pose");
    RwCameraSetViewWindow(camera.Camera(),RwCameraGetViewWindow(original));
    class Frame final : public INativeCombatWorldFrameServices {
    public:
        NativeCombatPlayerSceneActors& actors;unsigned body{},components{};
        explicit Frame(NativeCombatPlayerSceneActors& a):actors(a){}
        unsigned Address()const override{return reinterpret_cast<unsigned>(this);}
        void ProcessActorUpdates(unsigned camera)override{actors.RenderModels(camera,52u);++body;}
        unsigned UpdateActorComponents(unsigned camera)override{actors.RenderModels(camera,36u);++components;return 0x55u;}
    } frame(actors);
    const auto capture=[&](bool draw_actors){
        RwRGBA clear{17,31,47,255};check(RwCameraClear(camera.Camera(),&clear,rwCAMERACLEARIMAGE|rwCAMERACLEARZ)!=nullptr,"Player M01 clear");
        check(RwCameraBeginUpdate(camera.Camera())!=nullptr,"Player M01 begin");
        struct End {RwCamera* c;~End(){if(c)RwCameraEndUpdate(c);}} end{camera.Camera()};
        if(draw_actors)check(world.RenderFrame(addr(camera.Camera()),frame)==0x55u,"Player original world frame phases");else world.Render();
        IDirect3DSurface9* target{};check(SUCCEEDED(device->GetRenderTarget(0u,&target)),"Player M01 target");
        struct Surface {IDirect3DSurface9* p;~Surface(){if(p)p->Release();}} target_owner{target};
        D3DSURFACE_DESC desc{};check(SUCCEEDED(target->GetDesc(&desc)),"Player target description");
        IDirect3DSurface9* staging{};check(SUCCEEDED(device->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&staging,nullptr)),"Player readback surface");
        Surface staging_owner{staging};check(SUCCEEDED(device->GetRenderTargetData(target,staging)),"Player GPU readback");
        D3DLOCKED_RECT lock{};check(SUCCEEDED(staging->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"Player readback lock");
        std::vector<std::uint32_t> pixels(desc.Width*desc.Height);
        for(unsigned y=0;y<desc.Height;++y)std::memcpy(pixels.data()+y*desc.Width,static_cast<const char*>(lock.pBits)+y*lock.Pitch,desc.Width*4u);
        staging->UnlockRect();return pixels;
    };
    VerifyNativeCombatPlayerEntry(players,one,two,database,inventory,skill_records,skills,router,effects,packages);
    VerifyNativeCombatPlayerNewActor(source,pool,inventory,database,skill_records,skills,effects);
    check(players.StageCount()==2u && !actors.SceneChild(absent),"actual Player world enters two and skips absent child");
    unsigned unexpected_retirement{};
    VerifyNativeCombatFirstRoster(players,[&]{players.ClearActors([&](std::uint32_t){++unexpected_retirement;throw std::logic_error("unexpected first-cursor retirement");},[](std::uint32_t){});},"Player");
    check(!unexpected_retirement,"first-cursor guard precedes any Player retirement callback");
    const auto baseline=capture(false);
    actors.UpdateActors(0.02f);const auto rendered=capture(true);
    check(baseline.size()==rendered.size(),"Player M01 GPU captures require identical render target dimensions");
    unsigned differences{};for(std::size_t i=0;i<baseline.size();++i)differences+=baseline[i]!=rendered[i];
    check(differences>10u && frame.body==1u && frame.components==1u,"actual Player bodies/equipment change original M01 GPU output");
    check(word(one.Current().Address()+28u)==1u || word(two.Current().Address()+28u)==1u,"Player original frustum draw gate executed");
    {
        NativeCombatActorSkillRuntime first_skills(players.Actor(first),registry,inventory),
            second_skills(players.Actor(second),registry,inventory);
        (void)first_skills.RebuildStats();(void)second_skills.RebuildStats();
        first_skills.SetFacetState(1u,false);second_skills.SetFacetState(1u,false);
        (void)first_skills.SetCurrentStat(7u,first_skills.StatParameter(0u));
        (void)second_skills.SetCurrentStat(7u,second_skills.StatParameter(0u));
        const auto hp=first_skills.CurrentStat(7u);
        auto writable=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
        const auto primary=writable(first+636u),secondary=writable(first+648u);
        const auto second_primary=writable(second+636u),second_secondary=writable(second+648u);
        writable(first+636u)=0u;writable(first+648u)=0u;
        writable(second+636u)=0u;writable(second+648u)=0u;
        bool inspect=true,fail_second=false;NativeCombatPlayerMotion* running{};
        NativeCombatPlayerMotion motion(players,skills,router,[&](std::uint32_t a)->NativeCombatPlayerMotionBinding {
            if(inspect && a==first){inspect=false;reject([&]{one.Close();});reject([&]{running->Update(first,1u);});}
            if(a==first)return {one,first_skills};
            if(a==second){if(fail_second)throw std::logic_error("intentional Player motion service failure");return {two,second_skills};}
            throw std::invalid_argument("Player motion test foreign actor");
        });running=&motion;
        (void)one.Current().SetPosition({13.0f,0.0f,157.0f});
        motion.Update(first,0x100u);
        check(reinterpret_cast<const float*>(one.Current().Address()+172u)[0]==13.0f,
              "Player motion low flag byte alone controls formation reset");
        players.Actor(first).SetSlot(2u);motion.Update(first,1u);
        check(reinterpret_cast<const float*>(one.Current().Address()+172u)[0]==70.0f &&
              reinterpret_cast<const float*>(one.Current().Address()+160u)[1]==206.0f &&
              _stricmp(one.Current().Asset()->CurrentAction().c_str(),"z01")==0 && first_skills.CurrentStat(7u)==hp,
              "actual Player slot2 formation/yaw and idle preserve queried HP");
        writable(first+636u)=4u;motion.Update(first,0u);
        check(_stricmp(one.Current().Asset()->CurrentAction().c_str(),"z10")==0 && first_skills.PrimaryMotionState()==4u,
              "actual Player property636 chooses z10 without modifying state");
        writable(first+636u)=0u;writable(first+648u)=2u;motion.Update(first,0u);
        check(_stricmp(one.Current().Asset()->CurrentAction().c_str(),"z11")==0 && first_skills.SecondaryMotionState()==2u,
              "actual Player property648 chooses z11 without modifying state");
        writable(first+648u)=0u;motion.Update(first,0u);motion.Update(second,1u);
        *reinterpret_cast<std::uint8_t*>(first+1308u)=1u;
        *reinterpret_cast<std::uint8_t*>(second+1308u)=1u;
        fail_second=true;reject([&]{motion.Update(first,0u);});fail_second=false;
        check(!players.ActiveIterators(),"Player motion exception releases real stage iterator");
        *reinterpret_cast<std::uint8_t*>(first+1308u)=1u;
        motion.Update(first,1u);
        check(!*reinterpret_cast<const std::uint8_t*>(first+1308u) &&
              !*reinterpret_cast<const std::uint8_t*>(second+1308u) && !players.ActiveIterators(),
              "actual Player pending synchronization clears both stage flags and permits internal self refresh");
        VerifyNativeCombatActionLifecycle(first_skills,router,[&](std::uint32_t a,std::uint32_t flag){motion.Update(a,flag);},"Player");
        VerifyNativeCombatProtectionChildren(first_skills,actors,router,"Player");
        NativeCombatActorActionTasks playback(one,skills,router);
        VerifyNativeCombatActionWait(playback,first,[&](float delta){(void)one.Current().Update(delta,router);},"Player",router);
        VerifyNativeCombatReactionChildren(playback,first,[&](float delta){(void)one.Current().Update(delta,router);},
            [&](std::uint32_t a,std::uint32_t flag){motion.Update(a,flag);},"Player");
        VerifyNativeCombatMissActions(playback,second_skills,first_skills,registry,storage,skills,router,
            [&](float delta){(void)one.Current().Update(delta,router);},
            [&](std::uint32_t a,std::uint32_t flag){motion.Update(a,flag);},"Player",
            [&]{return HasNativeMissRecord(*one.Current().Asset());});
        VerifyNativeCombatFendOffImpact(second_skills,first_skills,registry,router,"Player");
        VerifyNativeCombatMessageChildren(playback,first_skills,[&]() -> NativeActorInstanceOwner& {return one.Current();},
            effects,router,[&](float delta){(void)one.Current().Update(delta,router);},
            *reinterpret_cast<float*>(game.Address()+1404u),"Player");
        VerifyNativeCombatReactionComposition(playback,second_skills,first_skills,registry,
            [&]() -> NativeActorInstanceOwner& {return one.Current();},effects,router,
            [&](float delta){(void)one.Current().Update(delta,router);},
            [&](std::uint32_t a,std::uint32_t flag){motion.Update(a,flag);},
            *reinterpret_cast<float*>(game.Address()+1404u),"Player",storage,skills);
        const std::array<NativeCombatActorSkillRuntime*,2> bindings{&first_skills,&second_skills};
        NativeCombatFormulaCatalog formulas(database);
        VerifyNativeCombatAttackActions(players,monster_actors,bindings,actors,storage,formulas,skills,router);
        writable(first+636u)=primary;writable(first+648u)=secondary;
        writable(second+636u)=second_primary;writable(second+648u)=second_secondary;
        players.Actor(first).SetSlot(0u);
        std::cout<<"Native5395E0 Player motion: actual2 M01 models, slot2 formation/yaw, HP query, z10/z11/z01, synchronous stage refresh, reentry/lifetime guards and iterator failure cleanup pass\n";
    }
    const auto original_child=one.Current().Address();
    const ResourceVec3 original_position{13.0f,0.0f,157.0f},original_rotation{0.0f,27.0f,0.0f};
    (void)one.Current().SetPosition(original_position);one.Current().SetRotation(original_rotation);
    unsigned rejoins{};bool fail_rejoin{};
    const auto observer=router.Subscribe(router.Key("re_join_scene"),3u,[&](std::uint32_t payload){
        if(word(payload)!=first+716u)return;
        ++rejoins;
        check(actors.SceneChild(first)==one.Current().Address(),"Player rejoin exposes current owned model");
        reject([&]{one.Close();});
        reject([&]{one.PlayAction("Z01",1u,skills,router);});
        if(fail_rejoin)throw std::logic_error("intentional Player action rejoin failure");
    });
    struct Observer {NativeScriptEventRouter& router;unsigned id;~Observer(){(void)router.Unsubscribe(id);}} remove{router,observer};
    one.PlayAction("Z01",1u,skills,router);
    check(one.Current().Address()==original_child && rejoins==0u,"existing Player action does not switch models");
    one.PlayAction("T01",1u,skills,router);
    const auto alternate=one.Current().Address();
    check(alternate!=original_child && one.Current().ModelName()=="101T01" && rejoins==1u && one.Count()==2u,
        "original539820 loads101T01 and rejoins the actual M01 world");
    check(!std::memcmp(reinterpret_cast<const void*>(alternate+172u),&original_position,sizeof(original_position)) &&
        !std::memcmp(reinterpret_cast<const void*>(alternate+160u),&original_rotation,sizeof(original_rotation)) &&
        word(alternate+492u)==first && word(alternate+516u)==first,
        "alternate Player model preserves actual transform and parent linkage");
    check(_stricmp(one.Current().Asset()->CurrentAction().c_str(),"T01")==0,"alternate original T01 clip selected");
    actors.UpdateActors(0.02f);(void)capture(true);
    one.PlayAction("Z01",1u,skills,router);
    check(one.Current().Address()==original_child && rejoins==1u &&
        _stricmp(one.Current().Asset()->CurrentAction().c_str(),"Z01")==0,
        "539350 cached fallback returns to requested101 and common action rereads replacement");
    fail_rejoin=true;
    reject([&]{one.PlayAction("T01",1u,skills,router);});
    check(one.Current().Address()==alternate && rejoins==2u && one.Count()==2u,
        "throwing rejoin keeps original preceding model/cache writes");
    fail_rejoin=false;
    one.PlayAction("Z01",1u,skills,router);
    one.PlayAction("T01",1u,skills,router);
    check(one.Current().Address()==alternate && rejoins==3u && one.Count()==2u,
        "539820 reuses associated alternate without duplicate allocation");
    std::cout<<"Native Player playback: original101 Z01,101T01 switch/rejoin, position/rotation/parent, cached fallback, alternate reuse and actual world update/render pass\n";
    VerifyNativeCombatPlayerModeledDeath(players.Actor(first),one,storage,database,inventory,skill_records,
        skills,router,world.GeometryQueue(),[&](float delta){actors.UpdateActors(delta);});
    VerifyNativeCombatPlayerModeledDeath(players.Actor(second),two,storage,database,inventory,skill_records,
        skills,router,world.GeometryQueue(),[&](float delta){actors.UpdateActors(delta);});
    (void)capture(true);
    const auto child=one.Current().Address();
    for(auto* cache:{&one,&two}) {
        const auto a=cache->ActorAddress();
        *reinterpret_cast<std::uint32_t*>(a+832u)=9u;
        const auto observe=router.Subscribe(router.Key("re_leave_stage"),1u,[&](std::uint32_t payload){
            check(word(payload)==a+716u && !word(a+832u),"Player retirement state832 precedes leave event");
            reject([&]{(void)RetireNativeCombatActor(players.Actor(a),effects.ContentHandles(),[]{});});
        });
        try{
            if(cache==&one){
                NativeCombatActorSkillRuntime conditional(players.Actor(a),registry,inventory);
                const auto id=word(a+1196u);const auto prior_rng=skills.Random();
                bool denied=false;
                try{(void)cache->LeaveStageDuringSkill(router,conditional);}catch(const std::logic_error&){denied=true;}
                check(denied,"Player stage leave rejects absent9126 permission");
                *reinterpret_cast<std::uint32_t*>(a+1196u)=2600u;
                *reinterpret_cast<std::uint32_t*>(a+832u)=2u;
                conditional.BindAllTargets([&](auto){return std::vector<std::uint32_t>{a};});
                conditional.BindConditionalRetirement([&]{
                    check(conditional.CurrentStat(7u)==0 && !conditional.ExitDispatchActive(),"9126 Player HP precedes actual retirement without death permission");
                    check(RetireNativeCombatActorDuringSkill(players.Actor(a),conditional,effects.ContentHandles(),
                        [&]{(void)cache->LeaveStageDuringSkill(router,conditional);})==6u,"9126 Player original retirement return6");
                });
                check(conditional.TriggerForTarget(7750u,a),"original7750 dispatches actual Player retirement inside own skill scope");
                *reinterpret_cast<std::uint32_t*>(a+1196u)=id;
                auto actual=skills.Random(),reference=prior_rng;
                for(unsigned n=0;n<624u;++n)check(actual.Next()==reference.Next(),"actual9126 Player retirement consumes no MT");
                std::cout<<"Native Combat9126 Player: original7750 real M01 cache/world leave, self dispatch permit and no MT pass\n";
            }else check(RetireNativeCombatActor(players.Actor(a),effects.ContentHandles(),[&]{(void)cache->LeaveStage(router);})==6u,
                       "Player original retirement return6");
        }
        catch(...){(void)router.Unsubscribe(observe);throw;}
        (void)router.Unsubscribe(observe);
        check(players.Actor(a).Slot()==6u && word(a+332u)==6u,"Player mirrored slot6 follows real world leave");
    }
    std::cout<<"Native52DE40 Player retirement: actual2 M01 model caches, state832, real stage/scene leave, callback guard and mirrored slot6 pass\n";
    check(!players.StageCount() && word(child+176u)==0x7f7fffffu,"Player actual world leave writes original hidden position");
    const auto published=storage.world_event_storage[0];storage.world_event_storage[0]=42u;
    reject([&]{scene.Close();});storage.world_event_storage[0]=published;
    scene.Close();
    check(!scene.SubscriptionCount() && std::all_of(storage.world_event_storage.begin(),storage.world_event_storage.end(),[](auto v){return !v;}),
          "WorldManager84 closes actual publication and subscriptions");
    std::cout<<"Native WorldManager prefix: actual84 four scene handle/key pairs, foreign-slot guard and retirement pass\n";
    actors.Close();manager_owner.CloseEvents();
    const std::array cache_bindings{std::pair{first,&one},std::pair{second,&two},std::pair{absent,&empty}};
    const auto cache_for=[&](std::uint32_t a)->NativeCombatPlayerModelCache& {
        for(const auto& [actor,cache]:cache_bindings)if(actor==a)return *cache;
        throw std::logic_error("Player clear missing actual cache");
    };
    unsigned retired{},detached{},retirement_readbacks{};bool fail_detach=true;
    const auto readback_token=router.Subscribe(router.Key("re_leave_stage"),3u,[&](std::uint32_t){
        const auto stage=players.CreateStageIterator();
        struct Stage{NativeCombatPlayerManager& p;std::uint32_t c;~Stage(){p.DeleteStageIterator(c);}} stage_guard{players,stage};
        const auto first=players.CreateFirstIterator();
        struct First{NativeCombatPlayerManager& p;std::uint32_t c;~First(){p.DeleteFirstIterator(c);}} first_guard{players,first};
        unsigned stage_count{},first_count{};
        while(!players.StageIteratorAtEnd(stage)){(void)players.Actor(players.StageIteratorActor(stage));players.NextStageIterator(stage);++stage_count;}
        while(!players.FirstIteratorAtEnd(first)){(void)players.Actor(players.FirstIteratorActor(first));players.NextFirstIterator(first);++first_count;}
        check(stage_count==players.StageCount() && first_count==3u,"retirement leave-stage callback reads stable stage and still-owned first roster");
        reject([&]{players.CreateActiveParty();});reject([&]{players.ClearActors([](auto){},[](auto){});});
        ++retirement_readbacks;
    });
    struct ReadbackObserver{NativeScriptEventRouter& r;std::uint32_t t;~ReadbackObserver(){(void)r.Unsubscribe(t);}} readback_observer{router,readback_token};
    const auto retire_actor=[&](std::uint32_t a){
        reject([&]{(void)players.AddPlayer(0u);});
        (void)RetireNativeCombatActor(players.Actor(a),effects.ContentHandles(),[&]{(void)cache_for(a).LeaveStage(router);});
        ++retired;
    };
    const auto detach_actor=[&](std::uint32_t a){
        check(retired==3u && players.Count()==3u,"all Player retirements precede every owner release");
        reject([&]{(void)players.CreateStageIterator();});reject([&]{(void)players.CreateFirstIterator();});
        cache_for(a).Close();
        if(fail_detach)throw std::logic_error("intentional Player clear detach failure");
        ++detached;
    };
    const auto cursor=players.CreateStageIterator();reject([&]{players.ClearActors(retire_actor,detach_actor);});
    players.DeleteStageIterator(cursor);
    reject([&]{players.ClearActors(retire_actor,detach_actor);});
    check(retired==3u && retirement_readbacks==3u && !players.ActiveIterators() && !detached && players.Count()==3u && players.SubscriptionCount()==3u,
          "failed detach retains all Player owners and subscriptions");
    reject([&]{players.CreateActiveParty();});reject([&]{(void)players.CreateStageIterator();});
    reject([&]{(void)players.CreateFirstIterator();});
    fail_detach=false;players.ClearActors(retire_actor,detach_actor);
    check(retired==3u && detached==3u && !players.Count() && !players.StageCount() && players.SubscriptionCount()==3u,
          "Player clear retry skips completed retirement and retains reusable collection subscriptions");
    players.ClearActors(retire_actor,detach_actor);
    players.CreateActiveParty();check(players.Count()>0u && !players.StageCount(),"same cleared Player manager recreates actual active party");
    std::cout<<"Native Player roster clear: actual3 owners, all retire before detach, active iterator/reentry guards, detach failure/retry,3 retained subscriptions and party recreation pass\n";
    players.Close();monster_actors.Close();pool.Clear();
    check(word(world.GeometryQueue()+4u)==word(world.GeometryQueue()+8u) &&
          cameras.ActiveCameraPointer()==active,"Player world drains queues and preserves active Scene camera");
    effects.ClearCombatEffects();manager_owner.Close();retire.active=false;
    check(!storage.ActorManagerSlot() && !manager_owner.Address() &&
        std::all_of(manager_image.words.begin(),manager_image.words.end(),[](auto v){return v==0u;}),
        "ActorManager owner clears publication and complete400-byte storage after unbind");
    std::cout<<"Native ActorManager owner: actual400 bytes, member424, ten events,15 game-object handlers, Player world/equipment and teardown pass\n";
    const auto retained_capacity=world.GeometryAllocations();camera.Close();world.Reset();
    translator.Reset();
    check(!translator.Loaded() && storage.target_selection.words.front()==0x13579bdfu &&
          storage.world_text_storage.back()==0x2468ace0u && storage.session_storage.front()==0x10203040u &&
          *reinterpret_cast<const std::uint8_t*>(addr(&storage)+517u)==0u,
          "embedded resource cleanup preserves neighboring CombatSystem storage and cold-init flag");
    check(world.GeometryAllocations()==retained_capacity && !world.TextAllocations() && !world.Loaded() &&
          word(world.Address()+200u)==word(world.Address()+204u) && word(world.Address()+216u)==word(world.Address()+220u) &&
          packages.SceneArchiveCount()==archives,"Player world reset releases archive/text and retains empty queue capacity until destruction");
    std::cout<<"Native Player world: original M01, actual2 stage models, SDK bodies/hands GPU differences="<<differences<<", frame2 phases, leave/teardown pass\n";
}
}
