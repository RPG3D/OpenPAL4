#pragma once
#include "pal4/native_arena_skill_manager_owner.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_arena_world_clump_binding.h"
#include "pal4/player_manager_object.h"
#include "native_particle_probe_camera.h"
#include <rwcore.h>
#include <rpworld.h>
#include <functional>
#include <iostream>
#include <algorithm>
#include <source_location>
namespace pal4::test {
inline unsigned SignpostWorldPixels(RpClump* clump,std::uint32_t arena){
    RpAtomic* first{};
    (void)RpClumpForAllAtomics(clump,[](RpAtomic* a,void* p)->RpAtomic*{
        auto*& first=*static_cast<RpAtomic**>(p);if(!first)first=a;return a;
    },&first);
    if(!first)throw std::runtime_error("signpost GPU model has no Atomic");
    const auto sphere=*RpAtomicGetWorldBoundingSphere(first);
    NativeParticleProbeCamera camera;
    struct WorldFixture {
        RpClump* clump;std::uint32_t arena;RwCamera* camera;
        RpWorld* world{};RpLight* light{};bool moved{},camera_added{},light_added{};
        ~WorldFixture(){
            if(moved){(void)RpWorldRemoveClump(world,clump);(void)NativeArenaWorldClumpBinding::Attach(arena,clump);}
            if(camera_added)(void)RpWorldRemoveCamera(world,camera);
            if(light_added)(void)RpWorldRemoveLight(world,light);
            if(light)(void)RpLightDestroy(light);
            if(world)(void)RpWorldDestroy(world);
        }
    } fixture{clump,arena,camera.camera};
    RwBBox bounds{{1000000.0f,1000000.0f,1000000.0f},{-1000000.0f,-1000000.0f,-1000000.0f}};
    fixture.world=RpWorldCreate(&bounds);fixture.light=RpLightCreate(rpLIGHTAMBIENT);
    if(!fixture.world || !fixture.light || !camera.camera || !camera.frame || !RpWorldUnlock(fixture.world))
        throw std::runtime_error("signpost GPU world allocation");
    const RwRGBAReal white{1.0f,1.0f,1.0f,1.0f};(void)RpLightSetColor(fixture.light,&white);
    if(!RpWorldAddLight(fixture.world,fixture.light))throw std::runtime_error("signpost GPU light binding");
    fixture.light_added=true;
    (void)RwCameraSetFrame(camera.camera,camera.frame);
    if(!RpWorldAddCamera(fixture.world,camera.camera))throw std::runtime_error("signpost GPU camera binding");
    fixture.camera_added=true;
    if(!NativeArenaWorldClumpBinding::Detach(arena,clump))throw std::runtime_error("signpost GPU temporary detach");
    if(!RpWorldAddClump(fixture.world,clump)){
        (void)NativeArenaWorldClumpBinding::Attach(arena,clump);
        throw std::runtime_error("signpost GPU world clump binding");
    }
    fixture.moved=true;
    const float distance=std::max(sphere.radius*3.0f,1.0f);
    camera.BeginAt({sphere.center.x,sphere.center.y,sphere.center.z-distance},0.01f,distance*4.0f+1.0f);
    (void)RwRenderStateSet(rwRENDERSTATEFOGENABLE,reinterpret_cast<void*>(FALSE));
    (void)RwRenderStateSet(rwRENDERSTATECULLMODE,reinterpret_cast<void*>(rwCULLMODECULLNONE));
    if(RpWorldRender(fixture.world)!=fixture.world)throw std::runtime_error("signpost GPU world render");
    return camera.EndPixels();
}
inline void VerifyNativeSignpostScene(NativeArenaSkillManagerOwner& owner,NativeCeguiUiSession& ui,
    HWND window,PlayerManagerObjectState& players,std::uint32_t arena,bool saved_marker_visible,bool saved_group_visible,
    const std::function<void(float,bool)>& frame){
    const auto check=[](bool v,const std::source_location at=std::source_location::current()){
        if(!v)throw std::runtime_error("native V signpost scene assertion at line "+std::to_string(at.line()));
    };
    check(!owner.SignpostActorCount());
    struct Restore {NativeArenaSkillManagerOwner& owner;NativeCeguiUiSession& ui;bool visible,group;
        ~Restore(){owner.Clear();(void)ui.SetSceneHudControlVisible(ui.MinimapMarkerButtonAddress(),visible);
            (void)ui.SetMinimapButtonGroupVisible(group);}
    } restore{owner,ui,saved_marker_visible,saved_group_visible};
    // Explicit protocol fixture: this scene normally hides arena actions.
    check(ui.SetMinimapButtonGroupVisible(true) &&
          ui.SetSceneHudControlVisible(ui.MinimapMarkerButtonAddress(),true));
    const auto press=[&]{
        check(ui.ProcessKeyboardMessage(window,WM_KEYDOWN,'V',0u));
        (void)ui.ProcessKeyboardMessage(window,WM_KEYUP,'V',0u);
    };
    {
        // Distinguish the selected slot from the original+A48 character id.
        // Restore this bounded fixture before any scene update or drawing.
        const auto selected=players.Word(0x2c50u/4u);check(selected<4u);
        const auto alternate=selected==0u?1u:0u;
        const auto action=players.Word((selected*0xb14u+0xa48u)/4u);check(action<4u);
        const auto old_action=players.Word((alternate*0xb14u+0xa48u)/4u);
        struct RestoreSelection {PlayerManagerObjectState& p;std::uint32_t selected,alternate,action;
            ~RestoreSelection(){p.Word(0x2c50u/4u)=selected;p.Word((alternate*0xb14u+0xa48u)/4u)=action;}
        } selection{players,selected,alternate,old_action};
        players.Word(0x2c50u/4u)=alternate;players.Word((alternate*0xb14u+0xa48u)/4u)=action;
        press();check(owner.SignpostActorCount()==1u);
        auto& memory=owner.Storage();
        const auto first_record=memory.ReadDword(memory.ReadDword(owner.Address()+20u));
        for(unsigned o=0;o<12u;o+=4u)
            check(memory.ReadDword(first_record+8u+o)==players.slots[action].base.Read(172u+o));
    }
    for(unsigned n=1;n<11u;++n)press();
    auto& m=owner.Storage();const auto manager=owner.Address();
    check(owner.SignpostActorCount()==11u && m.ReadDword(manager+52u)==10u);
    const auto record=m.ReadDword(m.ReadDword(manager+20u));
    const auto selected=players.Word(0x2c50u/4u);
    check(selected<4u);const auto character=players.Word((selected*0xb14u+0xa48u)/4u);
    check(character<4u);
    for(unsigned o=0;o<12u;o+=4u)check(m.ReadDword(record+8u+o)==players.slots[character].base.Read(172u+o));
    auto* clump=reinterpret_cast<RpClump*>(m.ReadDword(m.ReadDword(record+32u)+20u));
    const auto pixels=SignpostWorldPixels(clump,arena);check(pixels>0u && RpClumpGetWorld(clump));
    check(ui.SetSceneHudControlVisible(ui.MinimapMarkerButtonAddress(),false) &&
          ui.ProcessKeyboardMessage(window,WM_KEYDOWN,'V',0u) && owner.SignpostActorCount()==11u);
    (void)ui.ProcessKeyboardMessage(window,WM_KEYUP,'V',0u);
    const auto fade=m.ReadDword(manager+36u);const auto elapsed=m.ReadDword(fade+4u);
    frame(0.25f,true);check(m.ReadDword(fade+4u)==elapsed && owner.SignpostActorCount()==11u);
    frame(0.5f,false);check(owner.SignpostActorCount()==11u);
    frame(0.5f,false);check(owner.SignpostActorCount()==10u && m.ReadDword(manager+36u)==m.ReadDword(manager+40u));
    std::cout<<"Native V signpost scene: actual DLL selected-character dispatch, eleven placements, hidden gate, world GPU pixels="
             <<pixels<<", paused frame and live scene fade/cleanup pass\n";
}
}
