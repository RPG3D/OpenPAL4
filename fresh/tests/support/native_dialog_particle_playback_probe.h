#pragma once
#include "pal4/native_dialog_particle_playback.h"
#include "pal4/native_dialog_particle_effects.h"
#include "pal4/native_particle_effect_factory.h"
#include "pal4/native_particle_configuration.h"
#include "pal4/native_particle_group_frames.h"
#include "pal4/native_effect_texture_dictionary.h"
#include "pal4/native_effect_task_factory.h"
#include "pal4/native_dialog_task_runtime.h"
#include "pal4/native_dialog_object.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_script_object_runtime.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_package_archive.h"
#include "pal4/effect_system_render_state.h"
#include "pal4/particle_visual_object.h"
#include "native_particle_probe_camera.h"
#include <array>
#include <bit>
#include <filesystem>
#include <iostream>
#include <source_location>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeDialogParticlePlayback(const std::filesystem::path& game_root) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) {
            std::cerr << "native Dialog particle playback line " << at.line() << std::endl;
            throw std::runtime_error("native Dialog particle playback assertion");
        }
    };
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    NativeBaseObjectMemory memory;NativeEventDocumentRuntime documents(memory);
    NativeScriptObjectRuntime tasks(memory);
    std::array<std::uint32_t,15> storage{};const auto root=address(storage.data());
    (void)documents.InitializeDocument(root,"playback",0u);
    {
        struct Status final : INativeEffectTaskStatus {
            bool enabled{true};
            bool active{};
            bool EffectsEnabled() override{return enabled;}
            bool EffectIsActive(std::uint32_t) override{return active;}
        } status;
        std::array<std::uint32_t,42> system{};std::array<std::uint32_t,53> group{};group[1]=1u;
        NativeEffectTextureDictionary textures(game_root);
        NativeParticleEffectFactory objects(memory,documents,textures,address(system.data()),status);
        NativeEffectObjectQueues queues(memory,address(group.data()),objects);
        NativeEffectTaskFactory factory(memory,documents,tasks,objects);
        NativeDialogTaskRuntime scheduler(memory,documents,tasks,factory);
        NativeDialogObject dialog(memory,documents);
        NativeDialogParticleEffects activation(memory,dialog.Templates(),objects,queues);
        NativeDialogParticlePlayback effects(memory,objects,queues);
        NativeDialogPreDeleteRuntime deletion(memory,tasks,documents,dialog.Templates(),effects);
        NativeDialogResetRuntime reset(memory,tasks,effects);
        NativeDialogFrameRuntime frame(memory,tasks,reset,effects);
        NativeDialogPlaybackRuntime playback(memory,tasks,reset,activation.ActivationRuntime(),
            scheduler,activation.CleanupRuntime(),effects);
        struct Geometry final : IEffectSystemRenderQueueRuntime {
            NativeBaseObjectMemory& memory;
            explicit Geometry(NativeBaseObjectMemory& m):memory(m){}
            std::uint8_t ReadByte(std::uint32_t a) override{return memory.ReadByte(a);}
            std::uint32_t ReadWord(std::uint32_t a) override{return memory.ReadDword(a);}
            void SortAtomicGeometries(std::uint32_t) override{throw std::logic_error("particle-only playback has no clumps");}
        } geometry(memory);
        NativeParticleGroupFrames frames(memory,objects,0u,geometry);
        NativePackageArchive archive;check(archive.Open((game_root/"gamedata/Effect.cpk").string().c_str()));
        const auto data=archive.ReadMember("script\\foot_2.eft");
        check(data && documents.LoadBinaryDocument(root,*data) && dialog.Load(root));documents.CleanupDocument(root);
        auto& state=dialog.State();state.words[3]=address(group.data());
        std::uint32_t retained{},retained_frame{},resource{};
        const auto run=[&](bool interrupt) {
            check(DialogObject::StartPlayback(state,playback));
            const auto object=memory.ReadDword(state.words[28]+16u);
            check(object && objects.LiveObjects()==1u && system[41]==1u && queues.Count(176u)==1u);
            if(retained) check(object==retained && state.words[43]==retained_frame &&
                objects.Configuration(object).State().words[23]==resource);
            else {retained=object;retained_frame=state.words[43];resource=objects.Configuration(object).State().words[23];}
            check(RwFrameGetParent(reinterpret_cast<RwFrame*>(objects.Frame(object)))==reinterpret_cast<RwFrame*>(retained_frame));
            check(factory.LiveTasks()==3u && tasks.RegisteredTasks()==3u);
            auto& owner=objects.Configuration(object).State();
            auto& particle=*reinterpret_cast<ParticleVisualState*>(resource);
            // First start's +56 operation actually executes the shared500-step
            // preparation, rather than a fixture pause callback.
            owner.words[22]=std::bit_cast<std::uint32_t>(0.1f);
            check(DialogObject::PausePlayback(state,frame));owner.words[22]=0u;
            check(particle.ReadWord(204u)>0u);
            for(unsigned tick=0u;tick<80u && state.words[2];++tick) {
                (void)DialogObject::UpdatePlayback(state,0.05f,frame);
                frames.Update(address(group.data()),0.05);
                if(interrupt && tick==2u) {(void)DialogObject::ResetRuntimeState(state,reset);break;}
                if(tick==9u) {
                    NativeParticleProbeCamera camera;camera.Begin();frames.Draw(address(group.data()),0u);
                    check(camera.EndPixels()>0u);
                }
            }
            check(!state.words[2] && !queues.Count(176u) && !factory.LiveTasks() && !tasks.RegisteredTasks());
            for(auto p=state.words[24];p!=state.words[25];p+=8u) check(!memory.ReadDword(p+4u));
            check(!memory.ReadByte(object+6u) && objects.LiveObjects()==1u);
        };
        run(false);run(false);run(true);run(false);
        // Destroy while actual tasks, particles and SDK child frames are live.
        // Repeat after reload for both original disabled policy branches.
        const auto parent=RwFrameCreate();check(parent!=nullptr);
        for(unsigned mode=0u;mode!=3u;++mode) {
            if(mode) {
                check(documents.LoadBinaryDocument(root,*data) && dialog.Load(root));
                documents.CleanupDocument(root);state.words[3]=address(group.data());
            }
            check(DialogObject::StartPlayback(state,playback));
            check(factory.LiveTasks()==3u && objects.LiveObjects()==1u && queues.Count(176u)==1u);
            (void)DialogObject::UpdatePlayback(state,0.05f,frame);
            frames.Update(address(group.data()),0.05);
            check(RwFrameAddChild(parent,reinterpret_cast<RwFrame*>(state.words[43])));
            status.enabled=mode==0u;status.active=mode==2u;
            // Global policy and the group's queue gate are different fields.
            // Keep the group enabled until references have been removed.
            deletion.Prepare(dialog);
            check(!objects.LiveObjects() && !system[41] && !factory.LiveTasks() && !tasks.RegisteredTasks());
            unsigned children{};
            (void)RwFrameForAllChildren(parent,[](RwFrame* child,void* p)->RwFrame* {
                ++*static_cast<unsigned*>(p);return child;
            },&children);
            check(!queues.Count(176u) && !children);
            check(!state.words[2] && !state.words[3] && !state.words[43]);
            check(state.words[24]==state.words[25] && state.words[28]==state.words[29] &&
                  state.words[32]==state.words[33]);
            deletion.Prepare(dialog);
            status.enabled=true;status.active=false;
        }
        check(RwFrameDestroy(parent));
        std::cout << "Native Dialog particle playback: original foot_2 start/pre-render/tasks/update/draw/reset; "
                     "cached replay, interruption and active pre-delete/reload verified\n";
    }
    documents.DestroyDocument(root);check(!documents.DocumentCount() && !memory.OutstandingAllocations());
}
}
