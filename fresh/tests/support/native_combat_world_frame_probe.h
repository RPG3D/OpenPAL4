#pragma once
#include "pal4/native_combat_world_frame.h"
#include "native_game_world_render_state_probe.h"
#include "pal4/native_combat_system_renderer.h"
#include "pal4/native_combat_scene_frame.h"
#include "pal4/game_world_frame_processor.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_combat_world_resources.h"
#include "pal4/native_combat_player_scene_actors.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_world_asset.h"
#include "pal4/native_ui_world_camera.h"
#include "pal4/camera_data.h"
#include <rwcore.h>
#include <rpworld.h>
#include <d3d9.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <functional>
#include <fstream>
#include "pal4/native_combat_console.h"
#include "pal4/native_ui_text_image_overlay.h"
#include "pal4/native_ui_text_image_activities.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_gob_clump_asset.h"
#include "pal4/ui_text_image_animation.h"
#include <bit>

namespace pal4::test {
inline void VerifyNativeCombatWorldFrame(NativeCombatWorldResources& world,
    NativeCombatPlayerSceneActors& player_models,NativeCombatPlayerManager& players,
    NativeCombatMonsterManager& monsters,CameraDataState& data,NativeCombatSystemRenderer& renderer,
    NativeCombatSystemImage& image,NativeCeguiUiSession& ui,const std::function<void()>& restore_states,
    NativeCombatConsole& console,NativeUiTextImageOverlay& overlay,NativeEffectSceneSession& effects,
    const std::filesystem::path& root,NativeGameArenaManagerOwner& arena){
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    VerifyNativeGameWorldRenderState(arena);
    NativeUiWorldCamera camera;
    check(camera.Open(world.Asset().World(),320u,240u),"combined Combat world offscreen camera");
    auto* original=reinterpret_cast<RwCamera*>(data.At(0u));
    check(original && RwFrameTransform(RwCameraGetFrame(camera.Camera()),RwFrameGetLTM(RwCameraGetFrame(original)),rwCOMBINEREPLACE),"combined world actual CombatCamera pose");
    RwCameraSetViewWindow(camera.Camera(),RwCameraGetViewWindow(original));
    RwCameraSetNearClipPlane(camera.Camera(),RwCameraGetNearClipPlane(original));
    RwCameraSetFarClipPlane(camera.Camera(),RwCameraGetFarClipPlane(original));
    NativeCombatWorldFrame frame(player_models,monsters);
    bool rejected{};try{frame.Render(world,0u);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"null world-frame camera rejected before SDK draw");
    class RequiredSceneServices final : public IGameWorldFrameRuntime {
    public:
        unsigned attempts{},reentry_rejections{};NativeCombatSceneFrame* scene{};
        std::uint32_t ReadWord(std::uint32_t a)override{std::uint32_t v;std::memcpy(&v,reinterpret_cast<const void*>(a),4u);return v;}
        std::uint8_t ReadByte(std::uint32_t a)override{return *reinterpret_cast<const std::uint8_t*>(a);}
        std::uint32_t Call(GameWorldFrameOperation,std::uint32_t,std::uint32_t)override{
            ++attempts;try{scene->ProcessArenaFrame(scene->ArenaManager(),data_camera);}
            catch(const std::logic_error&){++reentry_rejections;}
            throw std::logic_error("complete native519C50 services are not bound");
        }
        std::uint32_t data_camera{};
    } required_scene;
    NativeCombatSceneFrame scene_frame(image,arena,required_scene);
    required_scene.scene=&scene_frame;required_scene.data_camera=reinterpret_cast<std::uint32_t>(camera.Camera());
    const auto capture=[&](unsigned actors){
        RwRGBA black{0,0,0,255};
        check(RwCameraClear(camera.Camera(),&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera.Camera()),"combined world camera begin");
        struct End{RwCamera* c;~End(){RwCameraEndUpdate(c);}} end{camera.Camera()};
        restore_states();
        if(actors>=2u){auto* sdk_camera=camera.Camera();if(actors==3u)CombatSceneFrame::Render(reinterpret_cast<std::uint32_t>(&sdk_camera),scene_frame,renderer);else renderer.Render(reinterpret_cast<std::uint32_t>(&sdk_camera));}
        else if(actors)frame.Render(world,reinterpret_cast<std::uint32_t>(camera.Camera()));else world.Render();
        auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
        IDirect3DSurface9* target{};IDirect3DSurface9* staging{};
        struct Surface{IDirect3DSurface9*& p;~Surface(){if(p)p->Release();}} target_owner{target},staging_owner{staging};
        check(device && SUCCEEDED(device->GetRenderTarget(0,&target)),"combined world GPU target");
        D3DSURFACE_DESC desc{};check(SUCCEEDED(target->GetDesc(&desc)) && (desc.Format==D3DFMT_A8R8G8B8 || desc.Format==D3DFMT_X8R8G8B8),"combined world GPU format");
        check(SUCCEEDED(device->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&staging,nullptr)) && SUCCEEDED(device->GetRenderTargetData(target,staging)),"combined world GPU readback");
        D3DLOCKED_RECT lock{};check(SUCCEEDED(staging->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"combined world GPU lock");
        std::vector<std::uint32_t> pixels(desc.Width*desc.Height);
        for(unsigned y=0;y<desc.Height;++y)std::memcpy(pixels.data()+y*desc.Width,static_cast<const char*>(lock.pBits)+y*lock.Pitch,desc.Width*4u);
        staging->UnlockRect();return pixels;
    };
    const auto baseline=capture(false),rendered=capture(true);
    unsigned changed{};for(std::size_t i=0;i<baseline.size();++i)changed+=(baseline[i]&0xffffffu)!=(rendered[i]&0xffffffu);
    check(changed>20u,"combined real Player/Monster models change world GPU pixels");
    const auto word=[](std::uint32_t a){std::uint32_t value;std::memcpy(&value,reinterpret_cast<const void*>(a),4u);return value;};
    unsigned visible_players{},visible_monsters{};
    for(std::size_t i=0;i<players.Count();++i){const auto child=word(players.ActorAt(i)+808u);if(child)visible_players+=word(child+28u)!=0u;}
    for(std::size_t i=0;i<monsters.Count();++i){const auto child=word(monsters.ActorAt(i)+808u);if(child)visible_monsters+=word(child+28u)!=0u;}
    check(visible_players && visible_monsters && !players.ActiveIterators() && !monsters.ActiveIterators(),"both actual rosters render and release first cursors");
    const auto queue=world.GeometryQueue();
    check(word(queue+4u)==word(queue+8u),"combined world transparent queue drains before component phase");
    const auto retry=capture(true);check(retry==rendered,"combined world frame repeats with stable actual GPU output");
    const auto state=image.State();const auto mode=image.Mode();auto& debug=reinterpret_cast<std::uint8_t*>(&image)[518u];const auto saved_debug=debug;
    struct Restore{NativeCombatSystemImage& i;std::uint32_t state;std::uint8_t mode,debug;~Restore(){i.State()=state;i.Mode()=mode;reinterpret_cast<std::uint8_t*>(&i)[518u]=debug;}} restore{image,state,mode,saved_debug};
    image.State()=1u;image.Mode()=0u;debug=0u;
    const bool overlay_mode=ui.OverlayQueueEnabled();const auto first_full=capture(3u);const auto full=capture(3u);
    unsigned first_differences{},xmin=0xffffffffu,ymin=0xffffffffu,xmax{},ymax{};
    for(std::size_t i=0;i<full.size();++i)if((first_full[i]&0xffffffu)!=(full[i]&0xffffffu)){++first_differences;auto x=static_cast<unsigned>(i%320u),y=static_cast<unsigned>(i/320u);xmin=std::min(xmin,x);ymin=std::min(ymin,y);xmax=std::max(xmax,x);ymax=std::max(ymax,y);}
    const auto save=[](const char* name,const std::vector<std::uint32_t>& pixels){
        std::ofstream out(name,std::ios::binary);out<<"P6\n320 240\n255\n";
        for(auto p:pixels){const char rgb[]{static_cast<char>(p>>16u),static_cast<char>(p>>8u),static_cast<char>(p)};out.write(rgb,3);}
        if(!out)throw std::runtime_error("Combat GPU evidence write failed");
    };
    save("H:/PAL4/objs/M12-system-render-20261021-first.ppm",first_full);
    save("H:/PAL4/objs/M12-system-render-20261021-repeat.ppm",full);
    std::cout<<"Combat same-state first repeat: differences="<<first_differences<<" bbox="<<xmin<<","<<ymin<<".."<<xmax<<","<<ymax<<"\n";

    check(ui.OverlayQueueEnabled()==overlay_mode && !ui.QueuedOverlayCount(),"full renderer restores queue ownership");
    for(unsigned s=2u;s<=6u;++s){image.State()=s;const auto next=capture(3u);unsigned differences{};for(std::size_t i=0;i<full.size();++i)differences+=(full[i]&0xffffffu)!=(next[i]&0xffffffu);std::cout<<"Combat render state"<<s<<" GPU_differences="<<differences<<"\n";check(!differences,"states1..6 preserve original full render routes");}
    check(!required_scene.attempts,"active CombatSceneFrame uses native renderer without touching missing arena services");
    const auto ui_before=ui.RenderCalls();
    for(unsigned attempt=0;attempt<2u;++attempt){
        image.State()=0u;auto* sdk_camera=camera.Camera();bool failed{};
        try{CombatSceneFrame::Render(reinterpret_cast<std::uint32_t>(&sdk_camera),scene_frame,renderer);}
        catch(const std::logic_error& e){failed=std::string_view(e.what())=="complete native519C50 services are not bound";}
        check(failed && ui.RenderCalls()==ui_before,"missing complete arena runtime rejects before Combat UI, with no reduced fallback");
    }
    check(required_scene.attempts==2u && required_scene.reentry_rejections==2u,"arena frame failure unwinds and permits retry; nested arena rejects");
    std::cout<<"Native Combat scene frame: active original58AF70 -> real576220; state0 missing complete519C50 rejects before UI, reentry/retry2 pass\n";
    image.State()=6u;
    image.Mode()=1u;const auto inactive=capture(2u);image.State()=0u;image.Mode()=0u;
    check(capture(2u)==inactive,"state0 and inactive mode both render actual UI/Console only");
    image.State()=7u;const auto closed=capture(2u);
    for(auto pixel:closed)check(!(pixel&0xffffffu),"state7 performs no draw");
    image.State()=1u;debug=1u;const auto diagnostic=capture(2u);unsigned diagnostic_pixels{};
    for(std::size_t i=0;i<full.size();++i)diagnostic_pixels+=(full[i]&0xffffffu)!=(diagnostic[i]&0xffffffu);
    check(diagnostic_pixels>20u,"real D3DX Combat debug lines produce GPU pixels");
    check(!ui.QueuedOverlayCount() && !players.ActiveIterators() && !monsters.ActiveIterators(),"complete renderer leaves no overlay/cursor ownership");
    debug=0u;
    check(!console.AnimationActivitiesBound(),"renderer Console probe begins with no borrowed activity owner");
    const auto texts=overlay.OwnedTexts();const auto contexts=effects.CombatXmlEffectCount();
    NativeBaseObjectMemory memory;NativeUiTextImageActivities activities(overlay,[&](std::uint32_t a){(void)memory.Free(a);});
    NativeGobClumpAsset weapon;check(weapon.Load(root,"gamedata\\PALWeapon\\WTA\\","WTA"),"renderer original WTA resource");
    struct Cleanup{NativeCombatConsole& c;NativeEffectSceneSession& e;EffectSystemHandleWords effect{};
        ~Cleanup(){try{c.Reset();c.BindAnimationActivity(nullptr);e.RemoveCombatXmlEffect(effect);}catch(...){}}} cleanup{console,effects};
    cleanup.effect=effects.LoadCombatXmlEffect("ZL-20",reinterpret_cast<std::uint32_t>(weapon.Clump()));
    check(effects.CombatXmlEffectReady(cleanup.effect) && effects.CombatXmlEffectCount()==contexts+1u,"full renderer has actual populated Combat XML owner");
    (void)effects.UpdateCombatEffects(0.05);
    console.BindAnimationActivity(&activities);
    const auto element=memory.Allocate(72u);(void)UiTextImageAnimation::ConstructMotion(element,0u,overlay);
    (void)UiTextImageOverlay::SetPosition(element,std::bit_cast<std::uint32_t>(160.f),std::bit_cast<std::uint32_t>(120.f),overlay);
    UiTextImageOverlay::SetImage(element,"zi0","luokong",0x3f800000u,overlay);
    console.AddAnimation(element,true,false);
    const auto populated=capture(2u);unsigned populated_pixels{};
    for(std::size_t i=0;i<full.size();++i)populated_pixels+=(full[i]&0xffffffu)!=(populated[i]&0xffffffu);
    check(populated_pixels>20u && !ui.QueuedOverlayCount(),"populated real effect and final Console stage produce full-frame pixels");
    console.Reset();console.BindAnimationActivity(nullptr);effects.RemoveCombatXmlEffect(cleanup.effect);
    check(!memory.OutstandingAllocations() && effects.CombatXmlEffectCount()==contexts && overlay.OwnedTexts()==texts,"renderer populated Console/effect owners retire independently");
    std::cout<<"Native populated combat renderer: Console=1 original_ZL20=1 GPU_changed="<<populated_pixels<<" allocations=0 pass\n";
    std::cout<<"Native combat system renderer: states0..7/mode, actual world/models/effects/actor/UI/Console routes, D3DX_debug_pixels="<<diagnostic_pixels<<" queues=0 pass\n";
    std::cout<<"Native combined combat world frame: Players="<<players.Count()<<" Monsters="<<monsters.Count()<<" visible="<<visible_players<<"/"<<visible_monsters<<" GPU_changed="<<changed<<" repeated=1 transparent_queue=0 cursors=0 pass\n";
}
}
