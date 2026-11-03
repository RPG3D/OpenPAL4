#pragma once
#include "pal4/native_script_player_actions_runtime.h"
#include "pal4/native_script_player_lock_runtime.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_actor_control_session.h"
#include "pal4/native_player_manager_session.h"
#include "pal4/native_player_animation_set.h"
#include "pal4/native_player_transform.h"
#include "pal4/game_object_property_access.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/effect_handle.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include <rpworld.h>
#include <bit>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativePlayerLeader(NativePlayerManagerSession& players,
    NativeScriptPlayerActionsRuntime& actions,NativeScriptPlayerLockRuntime& lock,
    NativeActorControlSession& control,NativePlayerAnimationSet& animations,
    IScriptPartyMembershipRuntime& party,
    IGameResourceManagerRuntime& transforms,std::uint32_t tree,
    const std::function<void(std::int32_t)>& mode,const std::function<bool()>& entered,
    INativeSkillSystemServices& skills,NativeEffectSceneSession& effects,NativePkFileManagerRuntime& packages) {
    const auto check=[](bool value,const char* why){if(!value)throw std::runtime_error(why);};
    auto& state=players.State();
    const auto address=[&](unsigned i){return reinterpret_cast<std::uint32_t>(&state.slots[i].base);};
    const auto word=[&](unsigned i,unsigned off)->std::uint32_t&{return state.Word((i*0xb14u+off)/4u);};
    check(state.Word(0x2c50u/4u)==0u && word(0,0xb08u) && !word(3,0xb08u),
        "leader fixture starts with actual Tianhe leader and inactive Ziying");
    const auto member1=word(1,0xb08u);
    std::array<std::array<std::uint32_t,6>,2> poses;
    for(unsigned i=0;i<2;++i)for(unsigned n=0;n<6;++n)poses[i][n]=word(i,0xa0u+n*4u);
    std::array<std::uint32_t,4> priority,player_state;
    for(unsigned i=0;i<4;++i){priority[i]=word(i,0x654u+167u*4u);player_state[i]=word(i,0xb0cu);}
    const auto visible1=word(1,0x1f0u);const auto old_mode=control.State().words[33u];
    const auto random=skills.Random();
    NativeAngelScriptEngine engine;NativeGameScriptServices services{};services.player_actions=&actions;services.party_membership=&party;
    NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    constexpr char source[]="void Same(){giPlayerSetLeader(0);} void Inactive(){giPlayerSetLeader(3);} "
        "void To1(){giPlayerSetLeader(1);} void To0(){giPlayerSetLeader(0);} "
        "void Join1(){giPlayerInTeam(1,false);} void Leave1(){giPlayerOutTeam(1,false);}";
    check(engine.Raw()->AddScriptSection("leader","leader.cs",source,sizeof(source)-1)>=0 &&
        engine.Raw()->Build("leader")>=0,"compile actual leader callback probe");
    asIScriptContext* raw{};check(engine.Raw()->CreateContext(&raw)>=0 && raw,"leader context");
    const std::unique_ptr<asIScriptContext,void(*)(asIScriptContext*)> context(raw,[](auto* p){p->Release();});
    const auto run=[&](const char* entry){
        check(raw->Prepare(engine.FunctionId("leader",entry))>=0,"prepare leader callback");
        if(raw->Execute()!=asEXECUTION_FINISHED)
            throw std::runtime_error(std::string("leader callback ")+entry+": "+
                (raw->GetExceptionString()?raw->GetExceptionString():"no script exception"));
    };
    const auto before=control.State();run("Same");
    check(control.State().words==before.words,"unchanged leader preserves all ActorControl words");
    run("Inactive");
    check(state.Word(0x2c50u/4u)==3u && control.State().words==before.words,
        "inactive target writes manager index but skips scene/control transition");
    state.Word(0x2c50u/4u)=0u; // Restore this isolated early-return fixture before a full switch.
    if(!member1)run("Join1");
    check(word(1,0xb08u)!=0u,"original party callback admits the fixture's Lingsha member");
    check(packages.AddEffectScriptAndProcess()!=0u,"flash resource lookup scope");
    const auto flash_file=packages.OpenEffectScriptPath("gamedata\\Effect\\script\\flash.eft");
    if(flash_file)check(packages.CloseEffectScriptMember(flash_file),"close original flash resource");
    (void)packages.CleanupAllScripts();
    const auto verify_flash=[&](unsigned i){
        const EffectSystemHandleWords handle{control.State().words[279],control.State().words[280]};
        check(control.State().words[33]==2u && *reinterpret_cast<const std::uint8_t*>(effects.SceneGroupAddress()+4u),
            "mode2 flash uses enabled real arena effect group");
        if(!flash_file){
            check(!handle.system && !EffectHandle::DialogNeedsUpdate(handle,effects.ContentHandles()),
                "absent original flash.eft retains original invalid-handle outcome");
            return;
        }
        check(EffectHandle::DialogNeedsUpdate(handle,effects.ContentHandles()),"mode2 owns an actual flash dialog");
        auto* frame=reinterpret_cast<RwFrame*>(EffectHandle::GetFrame(handle,effects.ContentHandles()));
        auto* clump=reinterpret_cast<RpClump*>(word(i,0x14u));
        check(frame && clump && RwFrameGetParent(frame)==RpClumpGetFrame(clump),"flash dialog attaches to current role RenderWare frame");
    };
    mode(2);verify_flash(0);run("To1");verify_flash(1);
    check(state.Word(0x2c50u/4u)==1u && control.State().words[1u]==address(1) &&
        control.State().words[19u]==1u && entered(),"leader1 binds actual control identity and adopts scene ownership");
    check(word(0,0x1f0u)==0u && word(0,0x568u)==0u && word(1,0x1f0u)==1u && word(1,0x568u)==tree,
        "leader transition removes old collision and adds actual new role");
    for(unsigned n=0;n<6;++n)check(word(1,0xa0u+n*4u)==poses[0][n],"leader copies old rotation and position through real transforms");
    check(animations.At(1).CurrentRecordName()=="C01" && word(1,0xb0cu)==player_state[0],
        "leader preserves original idle selection and player-state transfer");
    lock.SetActionCode(address(1)+0x170u,"C01");
    run("To0");verify_flash(0);mode(static_cast<std::int32_t>(old_mode));
    check(state.Word(0x2c50u/4u)==0u && control.State().words[1u]==address(0) && entered(),
        "reverse leader transition restores actual control and current-role owners");
    for(unsigned i=0;i<2;++i){
        NativePlayerTransform::Apply(address(i),8u,{poses[i][0],poses[i][1],poses[i][2]},0u,transforms);
        NativePlayerTransform::Apply(address(i),4u,{poses[i][3],poses[i][4],poses[i][5]},0u,transforms);
    }
    (void)ScriptPlayerActions::SetVisible(1,visible1!=0u,actions);
    if(!member1)run("Leave1");
    for(unsigned i=0;i<4;++i){
        GameObjectPropertyState property;std::memcpy(&property,reinterpret_cast<const void*>(address(i)+0x654u),sizeof(property));
        (void)GameObjectPropertyAccess::Set167And83(property,priority[i]);
        std::memcpy(reinterpret_cast<void*>(address(i)+0x654u),&property,sizeof(property));word(i,0xb0cu)=player_state[i];
    }
    auto actual=skills.Random(),expected=random;
    for(unsigned i=0;i<624;++i)check(actual.Next()==expected.Next(),"leader callbacks leave full MT unchanged");
    std::cout<<"Native leader: compiled same/inactive/0->1->0, mode2 previous animation, actual transforms/collision/control, nonzero lock action and full MT pass; flash resource="<<(flash_file?"present":"absent (original invalid-handle branch)")<<'\n';
}
}
