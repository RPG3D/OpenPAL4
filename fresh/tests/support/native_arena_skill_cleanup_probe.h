#pragma once
#include "pal4/native_arena_skill_manager_owner.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_gob_object_image.h"
#include "pal4/native_arena_world_clump_binding.h"
#include "pal4/script_arena_skill_state.h"
#include <rwcore.h>
#include <rpworld.h>
#include <stdexcept>

namespace pal4::test {
inline void CheckArenaSkill(bool value) {
    if(!value) throw std::runtime_error("native arena skill cleanup assertion");
}
inline void PopulateNativeArenaSkills(NativeEffectSceneSession& session,
    NativeArenaSkillManagerOwner* selected=nullptr) {
    auto& skills=selected?*selected:session.ArenaSkills(); auto& memory=skills.Storage();
    const auto manager=skills.Address();
    for(auto start:{4u,20u,36u}) {
        CheckArenaSkill(memory.ReadDword(manager+start)==memory.ReadDword(manager+start+4u));
        const auto stride=start==36u?12u:4u;
        auto array=memory.ReadDword(manager+start);
        if(!array) {
            array=memory.Allocate(stride);
            memory.WriteDword(manager+start,array);
            memory.WriteDword(manager+start+8u,array+stride);
        }
        const auto record=memory.Allocate(44u);
        for(auto offset=0u;offset<44u;offset+=4u) memory.WriteDword(record+offset,0u);
        const auto handle=session.GobEffects().Create("foot_2",0xffffffffu,false);
        CheckArenaSkill(handle.second && session.GobEffects().IsActive(handle.first,handle.second));
        memory.WriteDword(record+28u,0x12345678u);
        memory.WriteDword(record+36u,handle.first); memory.WriteDword(record+40u,handle.second);
        if(start==36u) {memory.WriteDword(array,0x3f800000u);memory.WriteDword(array+4u,0u);}
        memory.WriteDword(array+(start==36u?8u:0u),record);
        memory.WriteDword(manager+start+4u,array+stride);
    }
    memory.WriteDword(manager+48u,1u);memory.WriteDword(manager+52u,2u);
    CheckArenaSkill(memory.OutstandingAllocations()==7u);
}
inline void VerifyNativeArenaSkillsCleared(NativeEffectSceneSession& session) {
    auto& skills=session.ArenaSkills(); auto& memory=skills.Storage();
    for(auto start:{4u,20u,36u}) {
        const auto array=memory.ReadDword(skills.Address()+start);
        CheckArenaSkill(array && memory.ReadDword(skills.Address()+start+4u)==array &&
            memory.ReadDword(skills.Address()+start+8u)>array);
    }
    CheckArenaSkill(memory.ReadDword(skills.Address()+48u)==0u &&
        memory.ReadDword(skills.Address()+52u)==0u && memory.OutstandingAllocations()==4u);
}
inline void VerifyNativeArenaSkillLiveCleanup(NativeEffectSceneSession& session) {
    const auto active=session.ActiveCount(), pending=session.PendingCount();
    PopulateNativeArenaSkills(session);
    auto& skills=session.ArenaSkills(); auto& memory=skills.Storage();
    const auto record=memory.ReadDword(memory.ReadDword(skills.Address()+4u));
    memory.WriteDword(record+32u,0x1234u);
    bool rejected=false;
    try {skills.Clear();} catch(const std::logic_error&) {rejected=true;}
    CheckArenaSkill(rejected && memory.OutstandingAllocations()==7u && session.ActiveCount()==active+3u);
    memory.WriteDword(record+32u,0u);
    skills.Clear(); VerifyNativeArenaSkillsCleared(session);
    CheckArenaSkill(session.ActiveCount()==active && session.PendingCount()==pending+3u);
    skills.Clear(); VerifyNativeArenaSkillsCleared(session);
    NativeArenaSkillManagerOwner temporary(session.ContentHandles());
    PopulateNativeArenaSkills(session,&temporary);
    temporary.Destroy(); temporary.Destroy();
    CheckArenaSkill(!temporary.Address() && !temporary.Storage().OutstandingAllocations() &&
        session.ActiveCount()==active && session.PendingCount()==pending+6u);
}
inline void VerifyNativeArenaSkillHideRestore(NativeEffectSceneSession& session,std::uint32_t arena,
    const std::function<void()>& verify_resumed = {}) {
    const auto active=session.ActiveCount();
    PopulateNativeArenaSkills(session);
    auto& skills=session.ArenaSkills(); auto& memory=skills.Storage();
    const auto manager=skills.Address();
    const auto first=memory.ReadDword(memory.ReadDword(manager+4u));
    const auto retained=memory.ReadDword(memory.ReadDword(manager+20u));
    memory.WriteDword(first+32u,0x1234u);
    bool rejected=false;
    try {(void)ScriptArenaSkillState::HideSkillObject(skills);} catch(const std::logic_error&) {rejected=true;}
    CheckArenaSkill(rejected && memory.OutstandingAllocations()==7u && session.ActiveCount()==active+3u);
    memory.WriteDword(first+32u,0u);
    {
        struct RetainedActor {
            NativeBaseObjectMemory& memory;
            std::uint32_t record;
            RpClump* clump=RpClumpCreate();
            RwFrame* frame=RwFrameCreate();
            std::array<std::uint32_t,6> image{};
            ~RetainedActor() {
                memory.WriteDword(record+32u,0u);
                if(clump) {if(auto* world=RpClumpGetWorld(clump)) (void)RpWorldRemoveClump(world,clump); (void)RpClumpDestroy(clump);}
                if(frame) (void)RwFrameDestroyHierarchy(frame);
            }
        } actor{memory,retained};
        CheckArenaSkill(actor.clump && actor.frame);
        RpClumpSetFrame(actor.clump,actor.frame);
        actor.image[5]=reinterpret_cast<std::uint32_t>(actor.clump);
        memory.WriteDword(retained+32u,reinterpret_cast<std::uint32_t>(actor.image.data()));
        CheckArenaSkill(NativeArenaWorldClumpBinding::Attach(arena,actor.clump) && RpClumpGetWorld(actor.clump));
        (void)ScriptArenaSkillState::HideSkillObject(skills);
        CheckArenaSkill(!RpClumpGetWorld(actor.clump) && memory.OutstandingAllocations()==5u &&
            memory.ReadDword(manager+4u)==memory.ReadDword(manager+8u) &&
            memory.ReadDword(manager+36u)==memory.ReadDword(manager+40u) &&
            memory.ReadDword(manager+24u)-memory.ReadDword(manager+20u)==4u &&
            memory.ReadDword(memory.ReadDword(manager+20u))==retained && session.ActiveCount()==active);
        (void)ScriptArenaSkillState::HideSkillObject(skills);
        CheckArenaSkill(memory.OutstandingAllocations()==5u && !RpClumpGetWorld(actor.clump));
        // Exercise the original conditional resume branch with a live retained handle.
        const auto effect=session.GobEffects().Create("foot_2",0xffffffffu,false);
        CheckArenaSkill(effect.second!=0u);
        memory.WriteDword(retained+36u,effect.first); memory.WriteDword(retained+40u,effect.second);
        (void)ScriptArenaSkillState::ShowSignpost(skills);
        CheckArenaSkill(RpClumpGetWorld(actor.clump) && session.ActiveCount()==active+1u &&
            !session.GobEffects().IsActive(effect.first,effect.second) &&
            session.GobEffects().IsActive(memory.ReadDword(retained+36u),memory.ReadDword(retained+40u)));
        session.Update(0.016);
        session.Update(0.016);
        if(verify_resumed) verify_resumed();
        (void)ScriptArenaSkillState::HideSkillObject(skills);
        CheckArenaSkill(!RpClumpGetWorld(actor.clump) && session.ActiveCount()==active);
    }
    skills.Clear(); VerifyNativeArenaSkillsCleared(session);
}
}
