#pragma once
#include "pal4/native_combat_mouse_targets.h"
#include "pal4/native_combat_target_selection.h"
#include "pal4/native_combat_roster_targets.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_camera_screen_projection.h"
#include <rwcore.h>
#include <rpworld.h>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <functional>
#include <optional>
#include "pal4/native_combat_mouse_input.h"
namespace pal4::test {
inline void VerifyNativeCombatMouseTargets(NativeCombatMouseTargets& mouse,NativeCombatTargetSelection& selection,NativeCombatRosterTargets& targets,
    NativeCombatPlayerManager& players,NativeCombatMonsterManager& monsters,NativeScriptEventRouter& router,
    NativeCombatSystemImage& image,CameraDataState& camera,
    std::function<std::uint32_t(const std::array<float,2>&,const std::array<float,2>&)> dispatch={},
    NativeCombatMouseInput* input=nullptr,
    const std::function<void(const std::array<float,2>&,const std::array<float,2>&,
        const std::function<std::array<float,2>()>&)>& world_input_probe={}){
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto reject=[&](auto&& f){bool rejected{};try{f();}catch(const std::logic_error&){rejected=true;}check(rejected,"mouse target ownership/reentry guard");};
    const auto update=[&](const std::array<float,2>& tips,const std::array<float,2>& pixel){return dispatch?dispatch(tips,pixel):mouse.Update(tips,pixel);};
    const auto saved_words=image.target_selection.words;const auto saved_camera=camera;
    const auto state=image.State();const auto mode=image.Mode();
    bool started{};std::vector<std::uint32_t> observers;
    struct Restore{NativeScriptEventRouter& router;NativeCombatSystemImage& image;CameraDataState& camera;
        std::array<std::uint32_t,21> words;CameraDataState data;std::uint32_t state;std::uint8_t mode;bool& started;std::vector<std::uint32_t>& observers;
        ~Restore(){for(auto id:observers)(void)router.Unsubscribe(id);if(started){image.State()=3u;try{(void)router.Dispatch("stop_select_target",0u);}catch(...){}}
            image.target_selection.words=words;image.State()=state;image.Mode()=mode;camera=data;}
    } restore{router,image,camera,saved_words,saved_camera,state,mode,started,observers};
    check(players.StageCount() && monsters.StageCount()>1u,"mouse probe actual mixed stage rosters");
    auto* sdk_camera=reinterpret_cast<RwCamera*>(camera.At(0u));
    const auto start=[&](std::uint32_t actor){
        const std::array<std::uint32_t,4> payload{actor+716u,0u,0u,4u};
        image.State()=3u;image.Mode()=0u;started=true;
        (void)router.Dispatch("start_select_target",reinterpret_cast<std::uint32_t>(payload.data()));
        check(image.target_selection.words[18]==1u,"original Attack4 maps to opposite roster");
    };
    const auto stop=[&]{(void)router.Dispatch("stop_select_target",0u);started=false;};
    const auto try_pixel=[&](std::uint32_t wanted)->std::optional<std::array<float,2>>{
        // Projection reads the SDK view matrix; intersection reads the frame
        // LTM. Publish the current view after intervening camera updates.
        check(RwCameraBeginUpdate(sdk_camera)!=nullptr && RwCameraEndUpdate(sdk_camera)!=nullptr,"mouse probe current actual camera frustum");
        struct Samples{CameraDataState& camera;std::vector<std::array<float,2>> pixels;} samples{camera,{}};
        auto* clump=reinterpret_cast<RpClump*>(targets.ActorClumpAddress(wanted));
        check(clump!=nullptr,"mouse probe real actor child20 asset");
        RpClumpForAllAtomics(clump,[](RpAtomic* atomic,void* context){
            auto& s=*static_cast<Samples*>(context);auto* geometry=RpAtomicGetGeometry(atomic);
            auto* morph=RpGeometryGetMorphTarget(geometry,0);const auto* vertices=RpMorphTargetGetVertices(morph);
            const auto* triangles=RpGeometryGetTriangles(geometry);auto* matrix=RwFrameGetLTM(RpAtomicGetFrame(atomic));
            const auto count=RpGeometryGetNumTriangles(geometry);const auto stride=count>80?count/80:1;
            for(int i=0;i<count;i+=stride){
                const auto& t=triangles[i];RwV3d local{},world{};
                for(auto v:t.vertIndex){local.x+=vertices[v].x/3.f;local.y+=vertices[v].y/3.f;local.z+=vertices[v].z/3.f;}
                (void)RwV3dTransformPoint(&world,&local,matrix);
                auto pixel=ProjectNativeCameraScreen(s.camera,{world.x,world.y,world.z});
                pixel={std::round(pixel[0]),std::round(pixel[1])};
                auto* raster=RwCameraGetRaster(reinterpret_cast<RwCamera*>(s.camera.At(0u)));
                if(std::isfinite(pixel[0]) && std::isfinite(pixel[1]) && pixel[0]>=0.f && pixel[1]>=0.f &&
                   pixel[0]<RwRasterGetWidth(raster) && pixel[1]<RwRasterGetHeight(raster))s.pixels.push_back(pixel);
            }
            return atomic;
        },&samples);
        std::uint32_t last_hit{};
        for(auto p:samples.pixels)if((last_hit=mouse.FindNearest(p))==wanted)return p;
        const auto* matrix=RwFrameGetLTM(RpClumpGetFrame(clump));
        const auto* view=RwFrameGetLTM(RwCameraGetFrame(sdk_camera));
        std::cout<<"Mouse current sample miss: wanted="<<wanted<<" source="<<image.target_selection.words[10]
            <<" mode="<<image.target_selection.words[18]<<" samples="<<samples.pixels.size()<<" last_hit="<<last_hit
            <<" actor="<<matrix->pos.x<<","<<matrix->pos.y<<","<<matrix->pos.z
            <<" camera="<<view->pos.x<<","<<view->pos.y<<","<<view->pos.z<<std::endl;
        return std::nullopt;
    };
    const auto find_pixel=[&](std::uint32_t wanted){
        const auto pixel=try_pixel(wanted);
        if(!pixel)throw std::runtime_error("no projected triangle sample selects actual combat actor");
        return *pixel;
    };
    start(players.StageActorAt(0u));
    const auto initial=image.target_selection.words[16];
    const auto first=monsters.StageActorAt(0u),second=monsters.StageActorAt(1u);
    const auto p1=find_pixel(first),p2=find_pixel(second);
    image.target_selection.words[18]=0u;
    const auto same_player=players.StageActorAt(0u);const auto same_pixel=find_pixel(same_player);
    check(mouse.FindNearest(same_pixel)==same_player,"actual same-side Player pointer candidates");
    image.target_selection.words[18]=1u;
    {
        auto& health=*reinterpret_cast<std::uint32_t*>(first+648u);const auto value=health;
        struct HealthRestore{std::uint32_t& word;std::uint32_t value;~HealthRestore(){word=value;}} health_restore{health,value};
        health=3u;
        check(mouse.FindNearest(p1)!=first,"actual health3 candidate is rejected before clump query");
        auto* bytes=reinterpret_cast<std::uint8_t*>(image.target_selection.words.data());bytes[80u]=0u;
        check(mouse.FindNearest(p1)==first,"original allow-state3 gate permits the same actual candidate");bytes[80u]=1u;
    }
    check(image.target_selection.words[16]==initial && !targets.ActiveIterators() && !players.ActiveIterators() && !monsters.ActiveIterators(),
          "nearest actual Monster query has no visual mutation or leaked cursor");
    unsigned picked{},hidden{};bool fail_selection{};
    observers.push_back(router.Subscribe(router.Key("ae_ui_select"),1u,[&](std::uint32_t){
        reject([&]{(void)mouse.Update(p1,p1);});reject([&]{mouse.Close();});reject([&]{selection.Close();});reject([&]{targets.Close();});if(input){reject([&]{input->Close();});reject([&]{input->Restore(input->SavedCallback());});}++picked;
        if(fail_selection)throw std::runtime_error("intentional mouse selection observer failure");
    }));
    observers.push_back(router.Subscribe(router.Key("hide_monster_tips"),1u,[&](std::uint32_t){++hidden;}));
    check(update(p2,p2)==1u && image.target_selection.words[16]==second && picked==1u,
          "real574DF0/576190/53BC20 switches actual Monster and dispatches owned selection visuals");
    image.Mode()=1u;check(update(p1,p1)==2u && image.target_selection.words[16]==second && picked==1u,"inactive Combat render gate returns2 without selecting");image.Mode()=0u;
    const auto publication=image.target_selection.words[15];image.target_selection.words[15]^=4u;
    reject([&]{(void)update(p1,p1);});image.target_selection.words[15]=publication;
    check(update(p1,p1)==1u && image.target_selection.words[16]==first && picked==2u,"actual mouse selection reuses owners after rejected foreign camera");
    fail_selection=true;bool observer_threw{};
    try{(void)update(p2,p2);}catch(const std::runtime_error& e){observer_threw=std::string_view(e.what())=="intentional mouse selection observer failure";if(!observer_threw)throw;}
    fail_selection=false;
    check(observer_threw && image.target_selection.words[16]==second && !targets.ActiveIterators() &&
          !players.ActiveIterators() && !monsters.ActiveIterators(),"visual observer exception releases pointer/controller scopes and cursors");
    check(update(p1,p1)==1u && image.target_selection.words[16]==first,"actual pointer selection retries after observer exception");
    image.target_selection.words[17]=first;
    const std::array<float,2> miss{-10000.f,-10000.f};
    check(update(miss,miss)==1u && !image.target_selection.words[17] && hidden && image.target_selection.words[16]==first,
          "actual missed tips actor hides tips while nearest miss preserves selected target");
    stop();
    for(auto token:observers)(void)router.Unsubscribe(token);observers.clear();
    start(monsters.StageActorAt(0u));const auto player=players.StageActorAt(0u);const auto pp=find_pixel(player);
    check(mouse.FindNearest(pp)==player && !targets.ActiveIterators(),"actual Monster command scans opposite Player models");stop();
    if(world_input_probe){
        const std::function<std::array<float,2>()> current_target_pixel=[&]{
            // Other frame probes advance the actual model/camera. Reuse this
            // original-geometry search instead of retaining an old hit pixel.
            const auto mode=image.target_selection.words[18];
            struct RestoreMode{std::uint32_t& word;std::uint32_t value;~RestoreMode(){word=value;}}
                restore_mode{image.target_selection.words[18],mode};
            const auto source=targets.SelectionDialog(selection.Address()+40u);
            // The preceding case leaves a Monster source. Opposite/same is
            // relative to that real source, not an absolute Monster selector.
            bool source_player{};
            for(std::uint32_t i=0u;i<players.StageCount();++i)
                source_player|=players.StageActorAt(i)==source;
            // Full-frame Console camera animations may leave the first Monster
            // outside the view. Hover53C500 accepts actual actors from either
            // stage; require a fresh SDK intersection with a visible candidate.
            image.target_selection.words[18]=source_player?1u:0u;
            for(std::uint32_t i=0u;i<monsters.StageCount();++i)
                if(const auto pixel=try_pixel(monsters.StageActorAt(i)))return *pixel;
            image.target_selection.words[18]=source_player?0u:1u;
            for(std::uint32_t i=0u;i<players.StageCount();++i)
                if(const auto pixel=try_pixel(players.StageActorAt(i)))return *pixel;
            throw std::runtime_error("full frame hover requires an actual visible combat actor");
        };
        world_input_probe(p1,pp,current_target_pixel);
    }
    std::cout<<"Native combat mouse targets: actual Player+Monster child clumps, onscreen float triangles, same/opposite cursors, health3 gates, nearest/no visual mutation, render switch, inactive2, miss/hide, foreign camera, reentry and observer recovery pass\n";
}
}
