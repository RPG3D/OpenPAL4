#pragma once

#include "pal4/native_effect_object_pool.h"
#include "pal4/native_dialog_transform_runtime.h"
#include "native_dialog_preparation_fixture.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/dialog_object.h"
#include <rwcore.h>
#include <array>
#include <bit>
#include <cstring>
#include <cmath>
#include <map>
#include <functional>
#include <stdexcept>
#include <vector>

namespace pal4::test {
inline void VerifyNativeDialogTransforms() {
    const auto require=[](bool ok) { if (!ok) throw std::runtime_error("native Dialog transform regression"); };
    const auto close=[](float a,float b) { return std::fabs(a-b)<0.0001f; };
    NativeDialogTransformRuntime transforms;
    DialogObjectState dialog{};
    require(transforms.RefreshTransform(dialog)==0);
    auto* frame=RwFrameCreate();
    auto* parent=RwFrameCreate();
    require(frame && parent);
    dialog.words[43]=reinterpret_cast<std::uint32_t>(frame);
    (void)DialogObject::SetScale(dialog,{2,3,4},false,transforms);
    (void)DialogObject::SetRotation(dialog,{0,0,90},false,transforms);
    (void)DialogObject::SetPosition(dialog,{10,20,30},false,transforms);
    const auto local=*RwFrameGetMatrix(frame);
    require(close(local.right.x,0) && close(local.right.y,2) && close(local.right.z,0) &&
            close(local.up.x,-3) && close(local.up.y,0) && close(local.up.z,0) &&
            close(local.at.x,0) && close(local.at.y,0) && close(local.at.z,4) &&
            close(local.pos.x,10) && close(local.pos.y,20) && close(local.pos.z,30));
    const RwV3d translation{100,0,0};
    require(RwFrameTranslate(parent,&translation,rwCOMBINEREPLACE));
    (void)DialogObject::SetParentFrame(dialog,reinterpret_cast<std::uint32_t>(parent),transforms);
    (void)DialogObject::SetPosition(dialog,{1,2,3},true,transforms);
    (void)DialogObject::SetRotation(dialog,{0,0,45},true,transforms);
    const auto world=*RwFrameGetLTM(frame);
    require(close(world.pos.x,111) && close(world.pos.y,22) && close(world.pos.z,33) &&
            close(world.right.x,-std::sqrt(2.0f)) && close(world.right.y,std::sqrt(2.0f)));
    (void)DialogObject::SetScale(dialog,{1,1,1},true,transforms);
    require(close(std::bit_cast<float>(dialog.words[47]),3) &&
            close(std::bit_cast<float>(dialog.words[48]),4) &&
            close(std::bit_cast<float>(dialog.words[49]),5));
    (void)DialogObject::SetParentFrame(dialog,0,transforms);
    require(!RwFrameGetParent(frame) && close(RwFrameGetLTM(frame)->pos.x,11));
    dialog.words[53]=0xaabbcc01u; dialog.words[54]=123u;
    (void)DialogObject::ResetVisibility(dialog);
    require(dialog.words[53]==0xaabbcc00u && dialog.words[54]==0u);
    require(RwFrameDestroy(frame) && RwFrameDestroy(parent));
}
inline void VerifyNativeEffectPool() {
    VerifyNativeDialogTransforms();
    const auto require=[](bool ok) { if (!ok) throw std::runtime_error("native effect pool regression"); };
    NativeBaseObjectMemory memory;
    std::array<std::uint32_t,42> singleton{};
    const auto system=reinterpret_cast<std::uint32_t>(singleton.data());
    struct Objects final : INativeEffectPoolObjects {
        NativeBaseObjectMemory& memory;
        NativeDialogPreparationFixture preparation;
        std::uint32_t system;
        std::map<std::uint32_t,std::string> live;
        std::vector<std::uint32_t> destroyed;
        unsigned loads{};
        std::function<void()> on_destroy;
        Objects(NativeBaseObjectMemory& m, std::uint32_t s) : memory(m), preparation(m), system(s) {}
        void ClearDialogFacets(std::uint32_t effect) override { preparation.ClearFacets(effect); }
        std::string ReadEffectName(std::uint32_t document) override { return live.at(document-20u); }
        std::uint32_t LoadEffect(std::uint32_t manager, std::string_view name) override {
            if (manager!=system+68u) throw std::runtime_error("effect pool manager receiver");
            ++loads;
            if (name=="missing") return 0;
            if (name=="error") throw std::runtime_error("explicit load failure");
            const auto effect=memory.Allocate(sizeof(DialogObjectState));
            std::memset(reinterpret_cast<void*>(effect),0,sizeof(DialogObjectState));
            auto& state=*reinterpret_cast<DialogObjectState*>(effect);
            for (unsigned i=44;i<=52;++i) state.words[i]=std::bit_cast<std::uint32_t>(5.0f);
            state.words[53]=0xaabbcc01u; state.words[54]=9u;
            auto* frame=RwFrameCreate();
            if (!frame) throw std::runtime_error("effect pool frame");
            memory.WriteDword(effect+172u,reinterpret_cast<std::uint32_t>(frame));
            preparation.Initialize(state);
            live.emplace(effect,name); return effect;
        }
        void DestroyEffect(std::uint32_t effect) override {
            if (on_destroy) {
                auto callback=std::move(on_destroy);
                on_destroy={};
                callback();
            }
            if (live.erase(effect)!=1u || memory.ReadDword(effect+8u)!=0u)
                throw std::runtime_error("pool destroys each task-free Dialog once");
            auto* frame=reinterpret_cast<RwFrame*>(memory.ReadDword(effect+172u));
            if (RwFrameGetParent(frame) || !RwFrameDestroy(frame))
                throw std::runtime_error("pool detaches before payload destruction");
            preparation.Destroy(*reinterpret_cast<DialogObjectState*>(effect));
            destroyed.push_back(effect); (void)memory.Free(effect);
        }
    } objects(memory,system);
    auto pool=std::make_unique<NativeEffectObjectPool>(memory,system,objects,objects.preparation.documents);
    require(memory.OutstandingAllocations()==6u);
    require(pool->Acquire("disabled")==0u && objects.loads==0u);
    pool->Enable(3u);
    require(pool->Acquire("missing")==0u && singleton[30]==0u);
    const auto before_error=memory.OutstandingAllocations();
    bool failed=false;
    try { (void)pool->Acquire("error"); } catch (const std::runtime_error&) { failed=true; }
    require(failed && memory.OutstandingAllocations()==before_error);
    const auto first=pool->Acquire("fire"), second=pool->Acquire("ice"), third=pool->Acquire("fire");
    require(first && second && third && first!=third && singleton[30]==3u);
    const auto prepared_transform=[&](std::uint32_t effect) {
        const auto& state=*reinterpret_cast<const DialogObjectState*>(effect);
        const auto* matrix=RwFrameGetLTM(reinterpret_cast<RwFrame*>(state.words[43]));
        return objects.preparation.IsClean(state) && state.words[53]==0xaabbcc00u && state.words[54]==0u &&
            matrix->right.x==1.0f && matrix->up.y==1.0f && matrix->at.z==1.0f &&
            matrix->pos.x==0.0f && matrix->pos.y==1.0f && matrix->pos.z==1.0f;
    };
    require(prepared_transform(first) && prepared_transform(second) && prepared_transform(third));
    NativeDialogTransformRuntime transforms;
    (void)DialogObject::SetScale(*reinterpret_cast<DialogObjectState*>(first),{4,5,6},false,transforms);
    (void)DialogObject::SetRotation(*reinterpret_cast<DialogObjectState*>(first),{10,20,30},false,transforms);
    (void)DialogObject::SetPosition(*reinterpret_cast<DialogObjectState*>(first),{8,9,10},false,transforms);
    objects.preparation.Dirty(*reinterpret_cast<DialogObjectState*>(first));
    auto* parent=RwFrameCreate();
    auto* frame=reinterpret_cast<RwFrame*>(memory.ReadDword(first+172u));
    require(parent && RwFrameAddChild(parent,frame));
    pool->Return(first,0);
    require(!RwFrameGetParent(frame) && memory.ReadByte(first+4u)==1u &&
            memory.ReadByte(first+80u)==0u && singleton[26]==1u);
    pool->Return(second,0);
    require(singleton[26]==2u && objects.destroyed.empty());
    const auto reused=pool->Acquire("fire");
    require(reused==first && prepared_transform(reused) && singleton[26]==1u &&
            objects.loads==5u);
    pool->Return(reused,0);
    pool->Return(third,0);
    // Capacity is checked after insertion; >= limit evicts one, and name use
    // counts make ice the unique least-used candidate.
    require(singleton[26]==2u && singleton[30]==2u &&
            objects.destroyed==std::vector<std::uint32_t>{second});
    const auto retired=pool->Acquire("fire");
    require(retired==first || retired==third);
    pool->Return(retired,1);
    require(singleton[26]==1u && singleton[30]==1u && objects.live.size()==1u &&
            objects.destroyed.size()==2u);
    pool->Deinitialize();
    require(objects.live.empty() && objects.destroyed.size()==3u && singleton[26]==0u &&
            singleton[30]==0u && singleton[34]==0u && memory.OutstandingAllocations()==6u);
    pool->Enable(8u);
    // The original noncache path does not publish an active-tree entry.
    memory.WriteByte(system+85u,0u);
    const auto uncached=pool->Acquire("uncached");
    require(uncached && singleton[30]==0u);
    pool->Return(uncached,0);
    require(objects.live.empty() && objects.destroyed.size()==4u);
    pool->Enable(8u);
    const auto outer=pool->Acquire("outer"), inner=pool->Acquire("inner");
    objects.on_destroy=[&] { pool->Return(inner,1); };
    pool->Return(outer,1);
    require(objects.live.empty() && objects.destroyed.size()==6u && singleton[34]==2u);
    // The outer name remains valid across the nested callback. Shared scratch
    // storage would incorrectly insert an empty-name count after inner return.
    const auto name_head=singleton[32];
    const auto name_root=memory.ReadDword(name_head+4u);
    require(memory.ReadDword(name_root+20u)==5u && memory.ReadDword(name_root+28u)==1u);
    const auto name_other=name_root==memory.ReadDword(name_head) ?
        memory.ReadDword(name_root+8u) : memory.ReadDword(name_root);
    require(memory.ReadDword(name_other+20u)==5u && memory.ReadDword(name_other+28u)==1u);
    pool.reset();
    objects.preparation.Release();
    require(singleton[24]==0u && singleton[28]==0u && singleton[32]==0u &&
            memory.OutstandingAllocations()==0u && RwFrameDestroy(parent));
}
}  // namespace pal4::test
