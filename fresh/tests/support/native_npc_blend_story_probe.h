#pragma once
#include "pal4/native_arena_npc_scene_loader.h"
#include "pal4/native_script_npc_blend_runtime.h"
#include "pal4/native_angelscript_engine.h"
#include <rpworld.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeNpcBlendLabels(NativeArenaNpcSceneLoader& npcs,std::uint32_t arena) {
    NativeAngelScriptEngine engine;
    asIScriptContext* raw=nullptr;
    if(engine.Raw()->CreateContext(&raw)<0 || !raw)throw std::runtime_error("NPC label probe context");
    std::unique_ptr<asIScriptContext,void(*)(asIScriptContext*)> context(raw,[](auto* p){p->Release();});
    NativeScriptNpcBlendRuntime blend(npcs,arena,*raw);
    unsigned count{};
    npcs.Manager().ForEachObject([&](std::uint32_t npc) {
        const auto label=*reinterpret_cast<const std::uint32_t*>(npc+4u);
        if(!label)throw std::runtime_error("real scene NPC lost its LoadClump instance label");
        const auto name=blend.ActorName(npc);
        if(name.empty() || npcs.FindNpc(npcs.NpcManager(),name)!=npc ||
           !npcs.FindNpcInfo(npcs.NpcManager(),name))
            throw std::runtime_error("NPC fade-in label must resolve the same real actor and behavior info");
        ++count;return true;
    });
    if(!count)throw std::runtime_error("NPC label probe requires populated original scene");
}
// Observes the original M01 giNpcBlendOut("MN004",1.7,true).
class NativeNpcBlendStoryProbe final {
public:
    void Observe(NativeArenaNpcSceneLoader* npcs) {
        if(!npcs)return;
        const auto npc=npcs->Manager().FindObject("MN004");if(!npc)return;
        const auto& actor=*reinterpret_cast<const BaseObjectState*>(npc);
        auto* clump=reinterpret_cast<RpClump*>(actor.Read(0x14u));if(!clump)return;
        struct Colors {unsigned count{},low=255u,high{};} colors;
        RpClumpForAllAtomics(clump,[](RpAtomic* atomic,void* data)->RpAtomic* {
            auto& values=*static_cast<Colors*>(data);auto* geometry=RpAtomicGetGeometry(atomic);
            for(int i=0;i<RpGeometryGetNumMaterials(geometry);++i) {
                const auto alpha=RpMaterialGetColor(RpGeometryGetMaterial(geometry,i))->alpha;
                ++values.count;values.low=std::min(values.low,static_cast<unsigned>(alpha));
                values.high=std::max(values.high,static_cast<unsigned>(alpha));
            }
            return atomic;
        },&colors);
        if(actor.Read(0x1f0u) && colors.count && colors.low>0u && colors.high<255u) {
            if(colors.low!=colors.high || (fading_ && colors.low>previous_))
                throw std::runtime_error("M01 NPC fade materials disagree or alpha increases");
            if(!fading_)std::cout<<"Native M01 NPC blend: original MN004/1.7/wait=true changes actual SDK material alpha across frames\n";
            fading_=true;previous_=colors.low;++frames_;
        } else if(fading_ && !finished_ && !actor.Read(0x1f0u)) {
            if(!colors.count || colors.low!=255u || actor.Read(0x568u) || actor.Read(0x56cu) ||
                (!actor.Read(0x18u) && RpClumpGetWorld(clump)))
                throw std::runtime_error("M01 NPC fade did not restore colors/release scene and collision borrowers");
            finished_=true;
            std::cout<<"Native M01 NPC blend: actual alpha restored255, original NPC leave/collision cleanup and script wait resume; observed fade frames="<<frames_<<'\n';
        }
    }
    void RequireFade() const {
        if(!fading_||!finished_||frames_<2u)throw std::runtime_error("original M01 NPC fade not observed through completion");
    }
private:
    bool fading_{},finished_{};
    unsigned previous_=255u,frames_{};
};
}
