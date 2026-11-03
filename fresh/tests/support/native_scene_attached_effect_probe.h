#pragma once
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_monster_scene_object_builder.h"
#include "pal4/native_monster_info_reader.h"
#include "pal4/native_monster_object_owner.h"
#include "pal4/native_actor_user_data.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_monster_manager_owner.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_player_transform_runtime.h"
#include "pal4/base_object_attached_effect.h"
#include "pal4/effect_handle.h"
#include "native_particle_probe_camera.h"
#include <algorithm>

namespace pal4::test {
template<class Schema,class Animation>
void VerifyNativeSceneAttachedEffects(const std::filesystem::path& root,
    NativeBaseObjectMemory& memory,Schema& schema,Animation& animation) {
    const auto check=[](bool value) {
        if(!value) throw std::runtime_error("native original491 attached-effect lifecycle");
    };
    struct Directory {
        std::filesystem::path old=std::filesystem::current_path();
        ~Directory(){std::filesystem::current_path(old);}
    } directory;
    std::filesystem::current_path(root);
    NativePkFileManagerRuntime packages;
    std::array<std::uint32_t,20> filesystem{};filesystem[10]=0x1010u;
    packages.ProcessPackFiles(packages.InitializePackFileManager(),reinterpret_cast<std::uint32_t>(filesystem.data()));
    check(packages.LoadAllScripts()!=0u);
    NativeParticleProbeCamera camera;camera.Begin();(void)camera.EndPixels();
    NativeEffectSceneSession effects(root,packages);effects.BindCamera(reinterpret_cast<std::uint32_t>(camera.camera));
    NativeMonsterInfoReader reader;check(reader.Load(root,"M13","1"));
    const auto record=std::find_if(reader.Records().begin(),reader.Records().end(),
        [](const auto& row){return row.name=="trans01";});
    check(record!=reader.Records().end() && record->resource=="491");
    NativePlayerTransformRuntime transforms;
    NativeMonsterManagerOwner manager;check(manager.Construct()!=0u);
    NativeMonsterSceneObjectBuilder builder(root,manager,memory,schema,animation,transforms);
    check(manager.InitializeScene(std::span(&*record,1u),[&](const auto& row){builder.Build(row);}));
    auto& state=*reinterpret_cast<BaseObjectState*>(manager.FindObject("trans01"));
    // Red on the former borrowed-model path: neither the model name nor hook
    // was published, so the actual shared entry could never create491.
    check(state.Read(12u) && std::string_view(reinterpret_cast<const char*>(state.Read(12u)))=="491");
    auto* hook=NativeActorUserData::FindAtomic(reinterpret_cast<RpClump*>(state.Read(0x14u)),"hook_ef_bottom");
    check(hook && state.Read(0x530u)==reinterpret_cast<std::uint32_t>(hook));
    auto& runtime=effects.AttachedEffects();
    struct Cleanup {
        BaseObjectState& state;IBaseObjectAttachedEffectRuntime& runtime;NativeEffectSceneSession& effects;
        ~Cleanup(){(void)BaseObjectAttachedEffect::UpdateRotated(state,runtime);effects.Clear();}
    } cleanup{state,runtime,effects};
    check(BaseObjectAttachedEffect::SetupRotated(state,state.Read(12u),0xffffffffu,runtime)==1u);
    const EffectSystemHandleWords first{state.Read(0x53cu),state.Read(0x540u)};
    check(first.system==effects.SceneGroupAddress() && effects.ActiveCount()==1u);
    check(EffectHandle::GetName(first,effects.ContentHandles())=="491");
    const auto frame=EffectHandle::GetFrame(first,effects.ContentHandles());
    check(frame && RwFrameGetParent(reinterpret_cast<RwFrame*>(frame))==RpAtomicGetFrame(hook));
    check(*reinterpret_cast<const std::uint32_t*>(effects.SceneGroupAddress()+160u)==1u);
    check(BaseObjectAttachedEffect::SetupRotated(state,state.Read(12u),0xffffffffu,runtime)==1u);
    check(state.Read(0x53cu)!=first.id && effects.ActiveCount()==1u && effects.PendingCount()>=1u);
    check(BaseObjectAttachedEffect::UpdateRotated(state,runtime)==1u && effects.ActiveCount()==0u);
    // Disable the actor: enter must not fabricate a replacement instance.
    state.Write(0x1f0u,0u);
    const auto active=effects.ActiveCount();
    check(BaseObjectAttachedEffect::SetupRotated(state,state.Read(12u),0xffffffffu,runtime)==0u && effects.ActiveCount()==active);
    effects.Clear();
    std::cout<<"Native scene491 attached effect: original trans01 model/hook, parent, camera queue, replacement and retirement\n";
}
}
