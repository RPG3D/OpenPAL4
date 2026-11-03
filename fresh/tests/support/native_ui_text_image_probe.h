#pragma once
#include "pal4/native_ui_text_image_activities.h"
#include "pal4/native_ui_text_image_overlay.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_actor_instance_pool.h"
#include "pal4/native_combat_actor_screen_projection.h"
#include "pal4/native_combat_console_visual_events.h"
#include "pal4/native_combat_console.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_camera_manager_owner.h"
#include "pal4/native_combat_prerequisites.h"
#include "pal4/native_camera_screen_projection.h"
#include "pal4/game_resource_manager.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_ui_world_camera.h"
#include "pal4/ui_text_image_animation.h"
#include <rwcore.h>
#include <d3d9.h>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>

namespace pal4::test {
inline unsigned VerifyNativeUiTextImageActivities(const std::filesystem::path& root,
    NativeCeguiUiSession& ui,NativeCameraManagerOwner& cameras,
    NativeSkillRecordCatalog& skill_catalog,INativeSkillSystemServices& skill_system,
    std::uint32_t game,unsigned width,unsigned height) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto reject=[&](auto&& operation){bool rejected{};try{operation();}catch(const std::logic_error&){rejected=true;}check(rejected,"UI invalid request must reject");};
    NativeUiWorldCamera camera_owner;
    check(camera_owner.Open(nullptr,width,height),"floating UI owns a viewport-sized SDK offscreen camera");
    auto* camera=camera_owner.Camera();
    const auto initial_renders=ui.RenderCalls();
    check(ui.LoadImageset(root,"gamedata\\ui\\imagesets\\zi_0.imageset"),"native floating UI original zi0 imageset");
    NativeUiTextImageOverlay overlay(GetModuleHandleW(L"CEGUIBase.dll"),width,height);
    overlay.LoadCombatFonts();overlay.LoadCombatFonts();
    NativeBaseObjectMemory storage;
    unsigned freed{};
    NativeUiTextImageActivities activities(overlay,[&](unsigned a){
        check(overlay.ReadWord(a)==UiTextImageOverlay::kVtable && !overlay.ReadWord(a+8u),"object free follows text destructor");
        (void)storage.Free(a);++freed;
    });
    reject([&]{NativeUiTextImageActivities missing(overlay,{});});
    NativeGameDatabase db;check(db.Load(root),"floating UI original monster DB");
    NativeMonsterRecordCatalog catalog(db);
    NativeActorInstancePool pool([]{return false;},[&](std::string_view name){
        auto model=std::make_unique<NativeActorInstanceOwner>();
        (void)model->Construct(game);model->Load(root,name);return model;
    });
    NativeCombatActorModelOwner model(pool);(void)model.Construct(catalog,2579);
    auto& actor=model.Actor();
    const auto absent=ProjectNativeCombatActorScreen(actor,cameras,ui);
    check(absent[0]==std::numeric_limits<float>::max() && absent[1]==absent[0],
        "missing Combat child retains original projection sentinel");
    check(model.Attach("B01")==actor.Address(),"floating UI uses original attached B01 model");
    (void)SetupNativeCombatCamera(cameras);
    auto& combat_data=cameras.CameraDataForObject(cameras.FindByName("CombatCamera"));
    auto* combat_camera=reinterpret_cast<RwCamera*>(combat_data.At(0u));
    check(RwCameraBeginUpdate(combat_camera)!=nullptr,"Combat projection updates actual SDK view matrix");
    struct ProjectionEnd {RwCamera* p;~ProjectionEnd(){if(p)RwCameraEndUpdate(p);}} projection_end{combat_camera};
    const auto* pose=RwFrameGetLTM(RwCameraGetFrame(combat_camera));
    const auto p=pose->pos;const auto forward=pose->at;
    const ResourceVec3 anchor{p.x+forward.x*200.0f,p.y+forward.y*200.0f-80.0f,p.z+forward.z*200.0f};
    model.PositionSceneModel(anchor);
    const auto projected=ProjectNativeCombatActorScreen(actor,cameras,ui);
    const auto expected=ui.SceneScreenToUi(ProjectNativeCameraScreen(combat_data,{anchor.x,anchor.y+80.0f,anchor.z}));
    check(std::isfinite(projected[0]) && std::isfinite(projected[1]) &&
        std::fabs(projected[0]-expected[0])<0.001f && std::fabs(projected[1]-expected[1])<0.001f,
        "native actor projection uses child position, exact Y+80, CombatCamera and real UI transform");
    const auto center=ui.SceneScreenToUi({RwRasterGetWidth(RwCameraGetRaster(combat_camera))*0.5f,
        RwRasterGetHeight(RwCameraGetRaster(combat_camera))*0.5f});
    check(std::fabs(projected[0]-center[0])<0.01f && std::fabs(projected[1]-center[1])<0.01f,
        "actual camera forward ray projects to UI viewport center after head height adjustment");
    model.PositionSceneModel({anchor.x+pose->right.x*20.0f,anchor.y+pose->right.y*20.0f,anchor.z+pose->right.z*20.0f});
    const auto shifted=ProjectNativeCombatActorScreen(actor,cameras,ui);
    check(std::fabs(shifted[0]-projected[0])>1.0f && std::fabs(shifted[1]-projected[1])<0.01f,
        "moving real child along camera right changes projected X without moving Y");
    model.PositionSceneModel(anchor);
    check(RwCameraEndUpdate(combat_camera)!=nullptr,"Combat projection ends SDK update");projection_end.p=nullptr;
    actor.BindAnimationActivities(activities);
    struct Cleanup {
        NativeCombatActorOwner& actor;NativeCeguiUiSession& ui;
        ~Cleanup(){ui.ClearQueuedOverlays();try{actor.ClearAnimations();actor.UnbindAnimationActivities();}catch(...) {}}
    } cleanup{actor,ui};
    std::array<unsigned,5> objects{};
    const auto bits=[](float v){return std::bit_cast<unsigned>(v);};
    const auto value=[&](unsigned a,unsigned offset){return std::bit_cast<float>(overlay.ReadWord(a+offset));};
    for(unsigned i=0;i<objects.size();++i) {
        const auto a=objects[i]=storage.Allocate(i==0?68u:i==3?88u:i==4?92u:72u);
        if(i==0)(void)UiTextImageAnimation::Construct(a,0u,overlay);
        if(i==1)(void)UiTextImageAnimation::ConstructDelayed(a,0u,overlay);
        if(i==2 || i==4)(void)UiTextImageAnimation::ConstructMotion(a,0u,overlay);
        if(i==3)(void)UiTextImageAnimation::ConstructPulse(a,0u,overlay);
        const char* fonts[]{"RoleHpDec","MonsterHpDec","HpInc","MpInc","MonsterHpDec"};
        const auto text=i<2u || i==4u ? "-123" : "+123";
        (void)UiTextImageOverlay::SetTextAndFont(a,text,fonts[i],bits(1.0f),overlay);
        // Exercise host string replacement, reuse and empty cleanup as well.
        (void)UiTextImageOverlay::SetTextAndFont(a,"",fonts[i],bits(1.0f),overlay);
        (void)UiTextImageOverlay::SetTextAndFont(a,text,fonts[i],bits(1.0f),overlay);
        (void)UiTextImageOverlay::SetPosition(a,bits(width*(i+1u)/6.0f),bits(height*0.5f),overlay);
        if(i==4u)(void)UiTextImageOverlay::SetPosition(a,bits(projected[0]),bits(projected[1]),overlay);
        (void)UiTextImageAnimation::SetTiming(a,bits(0.05f),bits(0.15f),overlay);
        overlay.WriteWord(a+52u,bits(-1.0f));
        if(i==1)overlay.WriteWord(a+68u,bits(0.03f));
        if(i==2 || i==4)overlay.WriteWord(a+68u,bits(80.0f));
        if(i==2)UiTextImageOverlay::SetImage(a,"zi0","luokong",bits(1.0f),overlay);
        if(i==3)(void)UiTextImageAnimation::SetPulseTiming(a,bits(2.0f),bits(0.04f),bits(4.0f),bits(0.04f),overlay);
        if(i==4) {
            overlay.WriteWord(a,0x842698u);
            for(auto offset:{72u,76u})overlay.WriteWord(a+offset,bits(5.0f));
            for(auto offset:{80u,84u})overlay.WriteWord(a+offset,bits(0.1f));
            overlay.WriteWord(a+88u,0u);
        }
        activities.Validate(a);actor.AddAnimation(a,true);
    }
    check(overlay.OwnedTexts()==5u && !overlay.TemporaryStrings(),"native overlay owns five host strings and no DLL temporaries");
    actor.UpdateAnimations(0.02f);
    check(std::fabs(value(objects[2],32u)-(height*0.5f-1.6f))<0.001f &&
        std::fabs(value(objects[4],32u)-(projected[1]-1.6f))<0.001f,
        "motion and specialized damage both execute original motion base");
    check(std::fabs(value(objects[4],64u)-0.02f)<0.0001f && std::fabs(value(objects[4],40u)-1.1f)<0.0001f &&
        overlay.ReadWord(objects[4]+36u)==overlay.ReadWord(objects[4]+40u),"damage scale preserves base progress and synchronized text/image scale");
    check(value(objects[1],64u)==0.0f,"delayed activity has separate internal delay");
    actor.UpdateAnimations(0.02f);check(value(objects[1],64u)>0.0f,"delayed activity resumes through real dispatcher");
    auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    const auto capture=[&](bool draw,bool queued=false){
        const auto previous_queue_mode=ui.OverlayQueueEnabled();
        struct Mode {NativeCeguiUiSession& ui;bool mode;~Mode(){ui.SetOverlayQueueEnabled(mode);}} restore_mode{ui,previous_queue_mode};
        ui.SetOverlayQueueEnabled(queued);
        ui.ClearQueuedOverlays();RwRGBA black{0u,0u,0u,255u};
        check(RwCameraClear(camera,&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera),"floating UI begins real camera");
        struct End {RwCamera* camera;~End(){if(camera)RwCameraEndUpdate(camera);}} end{camera};
        D3DVIEWPORT9 viewport{};check(SUCCEEDED(device->GetViewport(&viewport)),"floating UI viewport read");
        std::cout<<"Floating UI viewport="<<viewport.Width<<'x'<<viewport.Height<<" raster="<<RwRasterGetWidth(RwCameraGetRaster(camera))<<'x'<<RwRasterGetHeight(RwCameraGetRaster(camera))<<'\n';
        RwUInt32 cull{};check(RwRenderStateGet(rwRENDERSTATECULLMODE,&cull)!=0,"floating UI saves enclosing cull mode");
        struct Cull {RwUInt32 saved;~Cull(){RwRenderStateSet(rwRENDERSTATECULLMODE,reinterpret_cast<void*>(saved));}} restore_cull{cull};
        check(RwRenderStateSet(rwRENDERSTATECULLMODE,reinterpret_cast<void*>(rwCULLMODECULLNONE))!=0,"floating UI enclosing screen phase disables backface culling");
        if(draw){
            const auto before=ui.QuadCalls();actor.DispatchAnimations(0u);
            std::cout<<"Floating UI dispatch quads="<<ui.QuadCalls()-before<<" queued="<<ui.QueuedOverlayCount()<<" enabled="<<ui.OverlayQueueEnabled()<<'\n';
            check(queued?ui.QueuedOverlayCount()>0u:ui.QueuedOverlayCount()==0u,"immediate and queued DLL paths retain original mode");
            if(queued)check(ui.RenderQueuedOverlays(),"floating UI flushes original DLL quads through shared GPU renderer");
        }
        IDirect3DSurface9* target{};check(SUCCEEDED(device->GetRenderTarget(0,&target)),"floating UI GPU target");
        struct Surface {IDirect3DSurface9* p{};~Surface(){if(p)p->Release();}} source{target},copy;
        D3DSURFACE_DESC desc{};check(SUCCEEDED(target->GetDesc(&desc)),"floating UI GPU description");
        std::cout<<"Floating UI target="<<desc.Width<<'x'<<desc.Height<<" configured="<<width<<'x'<<height<<" draw="<<draw<<'\n';
        check(RwCameraEndUpdate(camera)!=nullptr,"floating UI camera end");end.camera=nullptr;
        check(SUCCEEDED(device->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&copy.p,nullptr)) &&
            SUCCEEDED(device->GetRenderTargetData(target,copy.p)),"floating UI GPU readback");
        D3DLOCKED_RECT lock{};check(SUCCEEDED(copy.p->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"floating UI pixels lock");
        std::vector<unsigned> pixels(static_cast<std::size_t>(desc.Width)*desc.Height);
        for(unsigned y=0;y<desc.Height;++y)std::memcpy(pixels.data()+y*desc.Width,static_cast<const char*>(lock.pBits)+y*lock.Pitch,desc.Width*4u);
        copy.p->UnlockRect();ui.ClearQueuedOverlays();return pixels;
    };
    const auto blank=capture(false),solid=capture(true);unsigned changed{};unsigned long long light{};
    for(std::size_t i=0;i<solid.size();++i){if(blank[i]!=solid[i])++changed;for(auto shift:{0u,8u,16u})light+=(solid[i]>>shift)&255u;}
    std::cout<<"Floating UI changed="<<changed<<" light="<<light<<" font_height="<<value(overlay.ReadWord(objects[0]+20u),208u)<<'\n';
    check(changed>20u && ui.QuadCalls()>0u,"real CEGUI font and original miss image change GPU pixels");
    const auto image=overlay.ReadWord(objects[2]+24u);overlay.WriteWord(objects[2]+24u,0u);
    for(unsigned selected=0;selected<4u;++selected) {
        for(unsigned i=0;i<objects.size();++i)overlay.WriteWord(objects[i]+44u,bits(i==selected?1.0f:0.0f));
        const auto font_pixels=capture(true);unsigned count{};
        for(std::size_t i=0;i<font_pixels.size();++i)if(font_pixels[i]!=blank[i])++count;
        check(count>20u,"each original Combat numeric font independently changes GPU pixels");
        std::cout<<"Combat font GPU index="<<selected<<" pixels="<<count<<'\n';
    }
    overlay.WriteWord(objects[2]+24u,image);
    for(auto a:objects)overlay.WriteWord(a+44u,bits(1.0f));
    const auto batched=capture(true,true);unsigned batched_changed{};
    for(std::size_t i=0;i<batched.size();++i)if(batched[i]!=blank[i])++batched_changed;
    check(batched_changed>20u,"queued floating UI also changes real GPU pixels");
    for(auto a:objects)overlay.WriteWord(a+44u,bits(0.25f));
    const auto dim=capture(true);unsigned long long dim_light{};
    for(auto p:dim)for(auto shift:{0u,8u,16u})dim_light+=(p>>shift)&255u;
    check(dim_light<light && dim_light>0u,"real floating UI alpha changes GPU brightness");
    unsigned foreign=0x12345678u;reject([&]{activities.Validate(reinterpret_cast<unsigned>(&foreign));});
    reject([&]{activities.AdvanceAnimation(objects[0],-1.0f);});
    actor.UpdateAnimations(0.3f);
    check(!actor.AnimationCount() && freed==5u && !storage.OutstandingAllocations() && !overlay.OwnedTexts() && !overlay.TemporaryStrings(),
        "five real activities finish and retire text plus object allocations");
    actor.UnbindAnimationActivities();
    // The whole cold CombatSystem factory is still unbound. This fixture
    // explicitly injects its acquisition boundary and verifies real prerequisite
    // owners, without installing a default callback into production code.
    unsigned acquisitions{};bool fail_acquisition{},close_during_acquisition{};
    NativeCombatConsoleVisualEvents* visual_pointer{};
    reject([&]{NativeCombatConsoleVisualEvents missing(cameras,ui,overlay,{},{});});
    NativeCombatConsoleVisualEvents visual(cameras,ui,overlay,[&] {
        ++acquisitions;
        check(skill_system.Address()!=0u && cameras.CamerasReady() && ui.SystemAddress()!=0u,
            "visual event acquisition boundary requires actual initialized native prerequisites");
        if(fail_acquisition)throw std::logic_error("intentional Combat acquisition failure");
        if(close_during_acquisition)visual_pointer->Close();
    },[&](std::uint32_t a,bool){check(a==actor.Address(),"Flee hint receives validated native actor");});
    visual_pointer=&visual;
    NativeScriptEventRouter events;
    NativeCombatActorSkillRuntime skills(actor,events,skill_catalog,skill_system);
    (void)skills.RebuildStats();
    const auto max_hp=skills.StatParameter(0u),max_mp=skills.StatParameter(2u);
    std::cout<<"Combat visual original limits HP="<<max_hp<<" MP="<<max_mp<<'\n';
    check(max_hp>20 && max_mp>0,"original native actor stats for numeric events");
    (void)skills.SetCurrentStat(7u,max_hp);(void)skills.SetCurrentStat(9u,max_mp);
    visual.BindActor(actor,skills);
    NativeBaseObjectMemory console_memory;
    NativeCombatConsole console(console_memory,events);(void)console.Construct();
    console.BindDispatch([&](unsigned,unsigned entry,unsigned payload){visual.Dispatch(entry,payload);});
    console.Subscribe();
    struct EventCleanup {
        NativeCombatConsole& console;NativeCombatConsoleVisualEvents& visual;NativeCombatActorSkillRuntime& skills;
        ~EventCleanup(){console.BindDispatch({});console.Destroy();visual.Close();skills.Close();}
    } event_cleanup{console,visual,skills};
    std::array<unsigned,4> record{actor.Address()+716u,7u,0u,0u};
    const auto payload=reinterpret_cast<unsigned>(record.data());
    reject([&]{visual.Dispatch(0x540270u,payload);});
    visual.PrepareResources(root);visual.PrepareResources(root);
    reject([&]{visual.BindActor(actor,skills);});
    fail_acquisition=true;reject([&]{(void)events.Dispatch("ae_immunity_normal",payload);});fail_acquisition=false;
    close_during_acquisition=true;reject([&]{(void)events.Dispatch("ae_immunity_normal",payload);});close_during_acquisition=false;
    check(!actor.AnimationCount() && !visual.PendingAllocations(),"system acquisition failure precedes allocation");
    check(visual.BoundActors()==1u && actor.AnimationActivitiesBound(),"reentrant close cannot retire a live dispatch owner");
    std::array<unsigned,2> missed{actor.Address()+716u,actor.Address()};
    (void)events.Dispatch("ae_attack_missed",reinterpret_cast<unsigned>(missed.data()));
    (void)events.Dispatch("ae_immunity_normal",payload);
    (void)events.Dispatch("ae_react_attack",payload);
    check(actor.AnimationCount()==3u && visual.PendingAllocations()==3u,
        "three actual console subscriptions allocate original images in the actor-owned queue");
    const auto first_node=overlay.ReadWord(overlay.ReadWord(actor.Address()+732u));
    const auto first_image=overlay.ReadWord(first_node+12u);
    check(std::fabs(value(first_image,28u)-projected[0])<0.001f &&
        std::fabs(value(first_image,32u)-projected[1])<0.001f && overlay.ReadWord(first_image+24u)!=0u,
        "event-created image uses actual projected model head position and original zi0 resource");
    const auto image_frame=capture(true);unsigned image_pixels{};
    for(std::size_t i=0;i<image_frame.size();++i)if(image_frame[i]!=blank[i])++image_pixels;
    check(image_pixels>20u,"actual subscribed Combat image events change GPU pixels");
    actor.ClearAnimations();check(!visual.PendingAllocations() && !overlay.OwnedTexts(),"console image cancellation retires actual object/text owners");
    // Real setters emit me_attr_int_ext_changed synchronously through the same
    // router/console. No manual call to the numeric creator is used here.
    (void)skills.SetCurrentStat(7u,max_hp-10);
    check(actor.AnimationCount()==1u,"actual native HP setter event creates damage feedback");
    (void)skills.SetCurrentStat(7u,max_hp-5);
    (void)skills.SetCurrentStat(9u,max_mp-1);
    (void)skills.SetCurrentStat(9u,max_mp);
    check(actor.AnimationCount()==3u && visual.PendingAllocations()==3u && overlay.OwnedTexts()==3u,
        "actual HP damage/heal and positive MP event create three owned number animations");
    unsigned node=overlay.ReadWord(overlay.ReadWord(actor.Address()+732u));
    const char* expected_texts[]{"-10","+5","+1"};
    for(unsigned i=0;i<3u;++i) {
        const auto a=overlay.ReadWord(node+12u);const auto font_name=overlay.CreateCeguiString(
            i==0u?U"MonsterHpDec":i==1u?U"HpInc":U"MpInc");
        check(overlay.ReadWord(a)==0x842698u && overlay.ReadWord(a+20u)==overlay.GetFont(font_name) &&
            overlay.ConvertToUtf8(overlay.ReadWord(a+8u))==expected_texts[i],
            "event-created numeric object uses actual property delta and original font");
        overlay.DestroyCeguiString(font_name);
        check(i==0u ? value(a,52u)==0.0f : std::fabs(value(a,52u)-0.2f)<0.0001f,
            "number events use original queue stagger interval");
        node=overlay.ReadWord(node);
    }
    actor.UpdateAnimations(0.01f);actor.UpdateAnimations(0.25f);actor.UpdateAnimations(0.01f);
    const auto number_frame=capture(true);unsigned number_pixels{};
    for(std::size_t i=0;i<number_frame.size();++i)if(number_frame[i]!=blank[i])++number_pixels;
    check(number_pixels>20u,"actual native property events produce rendered damage/heal/MP numbers");
    reject([&]{visual.Dispatch(0x540100u,payload);});
    const auto saved=record[0];record[0]=0u;reject([&]{visual.Dispatch(0x540270u,payload);});record[0]=saved;
    check(actor.AnimationCount()==3u && visual.PendingAllocations()==3u,
        "unknown callback/foreign actor rejection preserves owned queue");
    actor.UpdateAnimations(2.0f);actor.UpdateAnimations(0.01f);
    check(!actor.AnimationCount() && !visual.PendingAllocations() && !overlay.OwnedTexts() && !overlay.TemporaryStrings(),
        "event-created activities finish through actual actor queue and release all native storage");
    (void)events.Dispatch("ae_immunity_normal",payload);
    visual.UnbindActor(actor.Address());
    check(!visual.BoundActors() && !visual.PendingAllocations() && !actor.AnimationCount() && !actor.AnimationActivitiesBound(),
        "unbinding cancels pending activities before releasing the borrowed actor");
    visual.BindActor(actor,skills);(void)events.Dispatch("ae_immunity_normal",payload);
    console.BindDispatch({});console.Destroy();visual.Close();skills.Close();
    check(!visual.BoundActors() && !actor.AnimationActivitiesBound() && !console_memory.OutstandingAllocations(),
        "native event presentation closes before borrowed actor/skill/UI owners");
    std::cout<<"Native Combat visual events: original console subscriptions, real HP/MP setters, three image callbacks, image_pixels="
        <<image_pixels<<" number_pixels="<<number_pixels<<" acquisition_calls="<<acquisitions<<", actor-owned queue and cleanup pass\n";
    model.Detach();
    check(ProjectNativeCombatActorScreen(actor,cameras,ui)==absent,"detached Combat model returns projection sentinel");
    pool.Clear();
    std::cout<<"Native floating UI: five vtables, motion/damage base, delayed/pulse, original zi0/luokong, GPU pixels="<<changed
        <<" queued_pixels="<<batched_changed<<" brightness="<<light<<"->"<<dim_light
        <<", original four Combat fonts, B01 projection="<<projected[0]<<','<<projected[1]<<", owned text/object cleanup pass\n";
    return ui.RenderCalls()-initial_renders;
}
}
