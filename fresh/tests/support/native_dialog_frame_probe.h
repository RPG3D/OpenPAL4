#pragma once
#include "pal4/native_dialog_frame_runtime.h"
#include "pal4/native_dialog_reset_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_script_object_runtime.h"
#include <rwcore.h>
#include <bit>
#include <memory>
#include <stdexcept>
#include <source_location>
#include <string>

namespace pal4::test {
inline void VerifyNativeDialogFrame() {
    const auto check=[](bool v, const std::source_location at=std::source_location::current()) {
        if (!v) throw std::runtime_error("native Dialog frame regression at line "+std::to_string(at.line()));
    };
    const auto address=[](const void* p) { return reinterpret_cast<std::uint32_t>(p); };
    NativeBaseObjectMemory memory;
    NativeScriptObjectRuntime tasks(memory);
    struct Effects final : INativeDialogResetEffects, INativeDialogPauseEffects {
        unsigned pauses{}, modes{}, releases{}, events{};
        bool EffectsEnabled() override { return true; }
        bool EffectIsActive(std::uint32_t) override { throw std::logic_error("enabled effect short circuit"); }
        void PauseEffect(std::uint32_t e) override {
            if(e!=123u) throw std::runtime_error("effect record +16");
            ++pauses;
        }
        void SetEffectMode(std::uint32_t e, std::uint32_t mode) override {
            if(e!=123u || mode!=0u) throw std::runtime_error("effect virtual +48 mode zero");
            ++modes;
        }
        void ReleaseEffectReferences(std::uint32_t) override { ++releases; }
        void ProcessEffectEvent(std::uint32_t, std::uint32_t) override { ++events; }
    } effects;
    NativeDialogResetRuntime reset(memory,tasks,effects);
    NativeDialogFrameRuntime runtime(memory,tasks,reset,effects);
    struct Task final : INativeScriptTaskLifecycle {
        NativeBaseObjectMemory& memory;
        NativeScriptObjectRuntime& tasks;
        unsigned begins{},ticks{},finishes{},deletes{};
        Task(NativeBaseObjectMemory& m,NativeScriptObjectRuntime& t):memory(m),tasks(t){}
        void Delete(std::uint32_t p,std::uint32_t flags) override {
            if(flags!=1u) throw std::runtime_error("task deletion flags");
            GomTaskBase base(tasks);
            (void)base.Destroy(*reinterpret_cast<GomTaskBaseState*>(p));
            (void)memory.Free(p); ++deletes;
        }
        void Begin(std::uint32_t) override { ++begins; }
        void Tick(std::uint32_t,float dt) override {
            if(dt!=0.25f) throw std::runtime_error("task elapsed forwarding");
            ++ticks;
        }
        bool IsDurationComplete(std::uint32_t) override { return ticks==2u; }
        void Finish(std::uint32_t) override { ++finishes; }
    };
    DialogObjectState state;
    check(!DialogObject::PausePlayback(state,runtime));
    check(DialogObject::UpdatePlayback(state,0.25f,runtime)==0);
    const auto container=memory.Allocate(sizeof(GomTaskContainerState));
    tasks.InitializeTaskContainer(container);
    state.words[2]=container;
    const auto task=memory.Allocate(sizeof(GomTaskBaseState));
    GomTaskBase base(tasks);
    (void)base.Construct(*reinterpret_cast<GomTaskBaseState*>(task));
    auto lifecycle=std::make_shared<Task>(memory,tasks);
    tasks.RegisterTask(task,lifecycle);
    tasks.AddTask(container,task,"native-dialog-frame",{3u,0.0f,0.0f});
    auto* parent=RwFrameCreate(); auto* tracked=RwFrameCreate(); auto* frame=RwFrameCreate();
    check(parent && tracked && frame && RwFrameAddChild(parent,tracked));
    RwV3d parent_pos{100,20,30}, local_pos{1,2,3};
    check(RwFrameTranslate(parent,&parent_pos,rwCOMBINEREPLACE) &&
          RwFrameTranslate(tracked,&local_pos,rwCOMBINEREPLACE));
    state.words[43]=address(frame);
    state.words[47]=state.words[48]=state.words[49]=std::bit_cast<std::uint32_t>(1.0f);
    state.words[53]=1u; state.words[54]=address(tracked);
    std::array<std::uint32_t,5> record{0u,0u,0u,0u,123u};
    state.words[28]=address(record.data()); state.words[29]=address(record.data()+record.size());
    state.words[20]=1u;
    check(DialogObject::PausePlayback(state,runtime) && effects.pauses==1u &&
          reinterpret_cast<GomTaskBaseState*>(task)->phase==2u);
    check(!DialogObject::PausePlayback(state,runtime) && effects.pauses==1u);
    (void)DialogObject::UpdatePlayback(state,0.25f,runtime);
    check(lifecycle->begins==1u && lifecycle->ticks==0u &&
          std::bit_cast<float>(state.words[44])==101.0f &&
          std::bit_cast<float>(state.words[45])==22.0f &&
          std::bit_cast<float>(state.words[46])==33.0f);
    const auto* ltm=RwFrameGetLTM(frame);
    check(ltm->pos.x==101.0f && ltm->pos.y==22.0f && ltm->pos.z==33.0f);
    state.words[20]|=0x10000u;
    check(DialogObject::UpdatePlayback(state,0.25f,runtime)==1 && lifecycle->ticks==0u);
    state.words[20]&=~0x10000u;
    (void)DialogObject::UpdatePlayback(state,0.25f,runtime);
    (void)DialogObject::UpdatePlayback(state,0.25f,runtime);
    check(lifecycle->ticks==2u && lifecycle->finishes==1u && state.words[2]!=0u);
    // Completion takes priority over tracking; an invalid tracked frame must
    // never be queried after the container has become ready.
    state.words[54]=0u;
    (void)DialogObject::UpdatePlayback(state,0.25f,runtime);
    check(!state.words[2] && lifecycle->deletes==1u && !tasks.RegisteredTasks() &&
          effects.pauses==1u && effects.modes==1u && effects.releases==1u && effects.events==1u &&
          memory.OutstandingAllocations()==0u);
    check(RwFrameDestroy(frame) && RwFrameRemoveChild(tracked) &&
          RwFrameDestroy(tracked) && RwFrameDestroy(parent));
}
}  // namespace pal4::test
