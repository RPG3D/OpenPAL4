#pragma once
#include "pal4/native_effect_support_objects.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/effect_random.h"
#include "pal4/ef_effect_system_object.h"
#include "native_particle_probe_camera.h"
#include <rpworld.h>
#include <bit>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace pal4::test {
inline void VerifyNativeEffectSupportObjects(NativeBaseObjectMemory& memory,
    NativeEffectSupportObjects& support,NativeEffectSupportSingletons& shared,
    std::uint32_t random,std::uint32_t auxiliary) {
    const auto check=[](bool ok) {if(!ok) throw std::runtime_error("native effect support assertion");};
    check(random && auxiliary && shared.random==random && shared.auxiliary==auxiliary &&
        memory.ReadDword(random)==EffectRandom::kVtable &&
        memory.ReadDword(auxiliary)==EfEffectSystemObject::kAuxiliaryVtableReady);
    const auto baseline=memory.OutstandingAllocations();
    std::uint32_t other_random{},other_auxiliary{};
    bool duplicate{};
    try {NativeEffectSupportObjects other(memory,other_random,other_auxiliary,shared,1u);}
    catch(const std::logic_error&){duplicate=true;}
    check(duplicate && !other_random && !other_auxiliary && shared.random==random &&
        shared.auxiliary==auxiliary && memory.OutstandingAllocations()==baseline);
    auto expected=memory.ReadDword(random+4u);
    expected=1194211693u*expected+12345u;
    check(support.NextBounded(97u)==expected%97u && memory.ReadDword(random+4u)==expected);
    expected=1194211693u*expected+12345u;
    check(support.NextBounded(0u)==0u && memory.ReadDword(random+4u)==expected);
    expected=1194211693u*expected+12345u;
    check(support.NextUnit()==double(expected%65536u)/65536.0);
    expected=1194211693u*expected+12345u;
    check(support.NextRange(7.0f,-3.0f)==7.0+double(expected%65536u)/65536.0*(-10.0));
    const auto first=memory.ReadDword(auxiliary+16u),second=memory.ReadDword(auxiliary+32u);
    check(first && second && first!=second && !memory.ReadByte(auxiliary+4u));
    const auto disabled_radius = support.RadiusSquared();
    (void)support.SetSquaredRadius(6.0f);
    check(!memory.ReadByte(auxiliary+4u) && support.RadiusSquared() == disabled_radius);
    // Native SDK atomics exercise real insert/erase rotations and callback
    // ownership, rather than seeding the tree with hand-written nodes.
    struct Models {
        std::vector<RpClump*> clumps;
        ~Models() {for(auto* clump:clumps) RpClumpDestroy(clump);}
    } models;
    struct Draw {
        static unsigned& Hits() {static unsigned hits{};return hits;}
        static RpAtomic* Render(RpAtomic* atomic) {++Hits();return atomic;}
    };
    for(unsigned i=0;i<9u;++i) {
        auto* clump=RpClumpCreate();check(clump);
        models.clumps.push_back(clump);
        auto* frame=RwFrameCreate();check(frame);RpClumpSetFrame(clump,frame);
        for(unsigned j=0;j<2u;++j) {
            auto* atomic=RpAtomicCreate();auto* child=RwFrameCreate();check(atomic && child);
            RwFrameAddChild(frame,child);RpAtomicSetFrame(atomic,child);
            RpAtomicSetRenderCallBack(atomic,Draw::Render);RpClumpAddAtomic(clump,atomic);
        }
    }
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    check(!support.RegisterClump(address(models.clumps[0])) && !memory.ReadDword(auxiliary+24u));
    support.SetRadius(5.0f);
    for(auto* clump:models.clumps) check(support.RegisterClump(address(clump)));
    check(memory.ReadDword(auxiliary+24u)==9u && memory.ReadDword(auxiliary+40u)==18u);
    (void)support.SetSquaredRadius(6.0f);
    check(support.RadiusSquared() == 36.0f && memory.ReadByte(auxiliary+4u) &&
        memory.ReadDword(auxiliary+24u)==9u && memory.ReadDword(auxiliary+40u)==18u);
    (void)support.SetSquaredRadius(5.0f);
    const auto registered=memory.OutstandingAllocations();
    for(auto* clump:models.clumps) check(support.RegisterClump(address(clump)));
    check(memory.OutstandingAllocations()==registered);
    RpAtomic* atomic{};
    RpClumpForAllAtomics(models.clumps[0],[](RpAtomic* a,void* p)->RpAtomic* {
        *static_cast<RpAtomic**>(p)=a;return nullptr;
    },&atomic);
    check(atomic && RpAtomicGetRenderCallBack(atomic)!=Draw::Render);
    {
        NativeParticleProbeCamera camera;camera.Begin();
        Draw::Hits()=0u;
        const RwV3d on_radius{3,4,-80},outside{3,4.01f,-80};
        RwFrameTranslate(RpAtomicGetFrame(atomic),&on_radius,rwCOMBINEREPLACE);
        (void)RpAtomicRender(atomic);check(Draw::Hits()==1u && !support.TakeRenderError());
        RwFrameTranslate(RpAtomicGetFrame(atomic),&outside,rwCOMBINEREPLACE);
        (void)RpAtomicRender(atomic);check(Draw::Hits()==1u && !support.TakeRenderError());
        // Moving the camera changes culling even though the Atomic stays put.
        const RwV3d near{3,4.01f,-80};RwFrameTranslate(camera.frame,&near,rwCOMBINEREPLACE);
        (void)RpAtomicRender(atomic);check(Draw::Hits()==2u && !support.TakeRenderError());
        (void)camera.EndPixels();
    }
    for(const auto index:{4u,0u,8u,2u,6u,1u,7u,3u,5u}) {
        auto* clump=models.clumps[index];support.UnregisterClump(address(clump));
        support.UnregisterClump(address(clump));
        bool restored=true;
        RpClumpForAllAtomics(clump,[](RpAtomic* a,void* output)->RpAtomic* {
            *static_cast<bool*>(output)&=RpAtomicGetRenderCallBack(a)==Draw::Render;
            return a;
        },&restored);check(restored);
    }
    check(!memory.ReadDword(auxiliary+24u) && !memory.ReadDword(auxiliary+40u) &&
        memory.OutstandingAllocations()==baseline);
    support.CleanupAuxiliary();
    const auto populate=[&] {
        for(const auto offset:{12u,28u}) {
            const auto tree=auxiliary+offset,head=memory.ReadDword(tree+4u);
            const auto nil=memory.ReadDword(head+4u);
            const auto size=offset==12u?20u:24u;
            const auto node=memory.Allocate(size);std::memset(reinterpret_cast<void*>(node),0,size);
            memory.WriteDword(node,nil);memory.WriteDword(node+4u,head);memory.WriteDword(node+8u,nil);
            memory.WriteDword(node+12u,0x1234u);
            if(offset==28u) memory.WriteDword(node+16u,0x5678u);
            memory.WriteDword(node+size-4u,1u);
            memory.WriteDword(head,node);memory.WriteDword(head+4u,node);memory.WriteDword(head+8u,node);
            memory.WriteDword(tree+12u,1u);
        }
    };
    support.SetRadius(1200.0f);
    check(memory.ReadByte(auxiliary+4u)==1u &&
        std::bit_cast<float>(memory.ReadDword(auxiliary+8u))==1440000.0f);
    populate();support.CleanupAuxiliary();support.CleanupAuxiliary();
    check(!memory.ReadByte(auxiliary+4u) && !memory.ReadDword(auxiliary+8u) &&
        !memory.ReadDword(auxiliary+24u) && !memory.ReadDword(auxiliary+40u) &&
        memory.ReadDword(auxiliary+16u)==first && memory.ReadDword(auxiliary+32u)==second &&
        memory.OutstandingAllocations()==baseline);
    // Leave populated inactive trees for the actual owner destructor's range
    // cleanup. This must release nodes even when the active cleanup gate is off.
    populate();
}
}  // namespace pal4::test
