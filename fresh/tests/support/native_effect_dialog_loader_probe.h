#pragma once
#include "pal4/rw_texture_dictionary.h"
#include "native_effect_support_probe.h"
#include "pal4/native_effect_factory_registry.h"
#include "pal4/native_effect_system_lifecycle.h"
#include "pal4/effect_system_factory_registry.h"
#include "pal4/effect_system_task_factory_registry.h"
#include "native_effect_named_dictionary_probe.h"
#include "pal4/native_effect_dialog_loader.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_effect_object_pool.h"
#include "pal4/native_effect_active_frame.h"
#include "pal4/native_effect_tree_storage.h"
#include "pal4/native_effect_handle_content_runtime.h"
#include "pal4/native_effect_owner_registry.h"
#include "pal4/native_player_transform_runtime.h"
#include "pal4/native_effect_group_lifecycle.h"
#include "pal4/native_effect_group_frame.h"
#include "pal4/native_effect_creation.h"
#include "pal4/native_script_effect_runtime.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_arena_effect_services.h"
#include "pal4/native_arena_effect_manager_owner.h"
#include "pal4/game_manager_reset.h"
#include <d3d9.h>
#include "native_dialog_particle_playback_probe.h"
#include "native_inline_effect_queue_probe.h"
#include "native_effect_context_probe.h"
#include "native_effect_xml_probe.h"
#include "native_nested_effect_probe.h"

namespace pal4::test {
inline void VerifyNativeEffectDialogLoader(const std::filesystem::path& game_root) {
    VerifyNativeInlineEffectQueues();
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) {std::cerr<<"effect Dialog loader line "<<at.line()<<std::endl;
            throw std::runtime_error("native effect Dialog loader assertion");}
    };
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    struct Directory {
        std::filesystem::path previous=std::filesystem::current_path();
        ~Directory(){std::filesystem::current_path(previous);}
    } directory;
    std::filesystem::current_path(game_root);
    NativeBaseObjectMemory memory;NativeEventDocumentRuntime documents(memory);
    NativeScriptObjectRuntime tasks(memory);
    NativeEffectSupportSingletons support_globals;
    {
        std::uint32_t random{},auxiliary{};
        NativeEffectSupportObjects fixture(memory,random,auxiliary,support_globals,0x12345u);
        VerifyNativeEffectSupportObjects(memory,fixture,support_globals,random,auxiliary);
    }
    check(!support_globals.random && !support_globals.auxiliary);
    {
        struct Status final : INativeEffectTaskStatus {
            bool EffectsEnabled() override{return true;}
            bool EffectIsActive(std::uint32_t) override{return false;}
        } status;
        std::array<std::uint32_t,42> system{};
        NativeEffectSupportObjects support(memory,system[35],system[36],support_globals,0x12345u);
        NativeEffectFactoryRegistry factories(memory,address(system.data()));
        bool duplicate_factories{};
        try {NativeEffectFactoryRegistry duplicate(memory,address(system.data()));}
        catch(const std::invalid_argument&){duplicate_factories=true;}
        check(duplicate_factories);
        check(!factories.FindObjectFactory("RtkPrtStdAtomicObject") &&
            !factories.FindTaskFactory("ShowobjTask"));
        const auto factory_baseline=memory.OutstandingAllocations();
        factories.RegisterAll();
        struct Lookup final : INativeEffectFactoryLookup {
            NativeEffectFactoryRegistry& registry;
            unsigned object_calls{},task_calls{};
            explicit Lookup(NativeEffectFactoryRegistry& r):registry(r){}
            std::uint32_t FindObjectFactory(std::string_view name) override {
                ++object_calls;return registry.FindObjectFactory(name);
            }
            std::uint32_t FindTaskFactory(std::string_view name) override {
                ++task_calls;return registry.FindTaskFactory(name);
            }
        } lookup(factories);
        check(system[5]==24u && system[9]==11u && system[3] && system[7] && system[3]!=system[7]);
        for(const auto& f:EffectSystemFactoryRegistry::kFactories)
            check(factories.FindObjectFactory(f.name)==f.creator);
        for(const auto& f:EffectSystemTaskFactoryRegistry::kFactories)
            check(factories.FindTaskFactory(f.name)==f.creator);
        check(!factories.FindObjectFactory("rtkprtstdatomicobject") &&
            !factories.FindTaskFactory("ShowobjTask_missing"));
        const auto registered_allocations=memory.OutstandingAllocations();
        factories.RegisterAll();
        check(memory.OutstandingAllocations()==registered_allocations && system[5]==24u && system[9]==11u);
        factories.Clear();factories.Clear();
        check(!system[5] && !system[9] && memory.OutstandingAllocations()==factory_baseline &&
            !factories.FindTaskFactory("ShowobjTask"));
        factories.RegisterAll();
        std::array<std::uint32_t,ArenaEffectManager::kObjectSize/4u> group{};
        const auto g=address(group.data());
        NativeEffectTreeStorage trees(memory);
        NativeEffectOwnerRegistry registry(memory,address(system.data()));
        NativeArenaEffectServices arena_services(memory,g);
        struct LifecycleServices final : INativeEffectGroupLifecycleServices {
            NativeArenaEffectServices& native;NativeEffectActiveFrame* active{};
            unsigned cleanups{},geometry_resets{};
            explicit LifecycleServices(NativeArenaEffectServices& n):native(n){}
            void CleanupEffects(std::uint32_t group) override {
                if(!active) throw std::logic_error("live group cleanup service is required");
                native.BindPlayback(*active);native.CleanupEffects(group);++cleanups;
            }
            void InitializeGeometryData(std::uint32_t a) override {
                native.InitializeGeometryData(a);
                ++geometry_resets;
            }
        } lifecycle_services(arena_services);
        ArenaEffectSharedState arena_shared;
        struct Scripts final : IGameManagerResetRuntime {
            NativeBaseObjectMemory& memory;std::uint32_t group;unsigned visits{};
            bool saw_live_group{},saw_script_disabled{};
            Scripts(NativeBaseObjectMemory& m,std::uint32_t g):memory(m),group(g){}
            void CallHandler(std::uint32_t,std::uint32_t) override {throw std::logic_error("CG handler not installed");}
            std::uint32_t FirstNode(std::uint32_t head) override {
                ++visits;saw_live_group=memory.ReadByte(group+4u)!=0u && memory.ReadDword(group+24u)!=0u;
                saw_script_disabled=memory.ReadByte(group+260u)==0u;
                return memory.ReadDword(head);
            }
            std::uint32_t NodeObject(std::uint32_t) override {throw std::logic_error("CG handler not installed");}
            void InvokeShutdown(std::uint32_t,std::uint32_t) override {throw std::logic_error("CG handler not installed");}
            void DeleteObject(std::uint32_t,std::uint8_t) override {throw std::logic_error("CG handler not installed");}
            std::uint32_t RemoveNode(ScreenEffectDispatcherState&,std::uint32_t) override {throw std::logic_error("CG handler not installed");}
            void CleanupCamera(std::uint32_t) override {throw std::logic_error("CG camera not installed");}
            std::uint32_t DestroyCameraRecord(std::uint32_t) override {throw std::logic_error("CG camera not installed");}
        } scripts(memory,g);
        NativeArenaEffectManagerOwner manager(memory,g,arena_shared,trees,address(system.data()),
            registry,lifecycle_services,scripts);
        auto& lifecycle=manager.Group();
        check(manager.Address()==g && group[0]==ArenaEffectManager::kVtable &&
            arena_shared.instance_count==1u && arena_shared.sentinel && group[71]);
        check(memory.ReadDword(group[71]+4u)==arena_shared.sentinel &&
            memory.ReadDword(group[71])==group[71] && memory.ReadDword(group[71]+8u)==group[71]);
        check(trees.ContextNil() && trees.TimedNil() && !group[1] && !registry.Contains(g));
        {
            const auto nils=std::array{trees.ActiveNil(),trees.PendingNil(),trees.InlineNil(),
                trees.ContextNil(),trees.TimedNil()};
            std::array<std::uint32_t,ArenaEffectManager::kObjectSize/4u> other{};
            const auto arena_nil=arena_shared.sentinel;
            const auto allocations_before_other=memory.OutstandingAllocations();
            NativeArenaEffectManagerOwner other_manager(memory,address(other.data()),arena_shared,trees,
                address(system.data()),registry,lifecycle_services,scripts);
            check(arena_shared.instance_count==2u && arena_shared.sentinel==arena_nil);
            // A populated disabled dispatcher owns its tree nodes; original
            // cleanup frees them while preserving the other manager's nil.
            const auto script_node=memory.Allocate(24u),script_head=other[71];
            check(script_node!=0u);
            memory.WriteDword(script_node,arena_nil);
            memory.WriteDword(script_node+4u,script_head);
            memory.WriteDword(script_node+8u,arena_nil);
            memory.WriteDword(script_node+12u,1u);
            memory.WriteDword(script_node+16u,0u);
            memory.WriteDword(script_node+20u,1u);
            memory.WriteDword(script_head,script_node);
            memory.WriteDword(script_head+4u,script_node);
            memory.WriteDword(script_head+8u,script_node);
            other[73]=1u;
            other_manager.Destroy();other_manager.Destroy();
            check(arena_shared.instance_count==1u && arena_shared.sentinel==arena_nil && !other[71]);
            check(memory.OutstandingAllocations()==allocations_before_other);
            bool destroyed_group{};
            try {(void)other_manager.Group();}
            catch(const std::logic_error&){destroyed_group=true;}
            check(destroyed_group);
            check(nils==std::array{trees.ActiveNil(),trees.PendingNil(),trees.InlineNil(),
                trees.ContextNil(),trees.TimedNil()});
        }
        check(!lifecycle.Enable(1u)); // disabled singleton
        // Clear real populated geometry storage without releasing capacity.
        // The group's actual destructor owns and frees this allocation later.
        group[14]=memory.Allocate(16u);check(group[14]!=0u);
        group[15]=group[16]=group[14]+16u;
        const auto geometry_buffer=group[14];
        arena_services.InitializeGeometryData(g+52u);
        check(group[14]==geometry_buffer && group[15]==geometry_buffer && group[16]==geometry_buffer+16u);
        EffectSystemHandle owner_handle{17u,g};
        arena_services.Created(g,owner_handle,{3.0f,4.0f,5.0f},1u);
        arena_services.Started(g,owner_handle,1);
        arena_services.Finished(g,owner_handle,1);
        arena_services.Removed(g,owner_handle,1u);
        check(owner_handle.id==17u && owner_handle.system==g && !group[6]);
        bool missing_playback{};
        try {arena_services.CleanupEffects(g);}
        catch(const std::logic_error&){missing_playback=true;}
        check(missing_playback);
        // +40 is the caller-owned dictionary; +52 belongs to the distinct
        // named dictionary tree and must not be overwritten by this owner.
        auto& dictionary=*reinterpret_cast<RwTextureDictionaryState*>(system.data()+10u);
        NativeEffectTextureDictionary textures(game_root,dictionary);
        NativeEffectNamedDictionarySharedState named_shared;
        auto& named=*reinterpret_cast<RwTexDictionaryTreeState*>(system.data()+13u);
        NativeEffectNamedDictionaryTree named_owner(memory,named,named_shared);
        const auto named_head=named.head;
        VerifyNativeEffectNamedDictionaryTree(memory,named_owner,named,named_shared);
        check(system[10]==RwTextureDictionary::kVtableAddress && !system[11] && system[12]==1u &&
            named.head==named_head && !named.count);
        bool premature_texture_lookup{};
        try {(void)textures.Find("gamedata\\effect\\tex\\unused.png");}
        catch(const std::logic_error&){premature_texture_lookup=true;}
        check(premature_texture_lookup);
        bool duplicate_dictionary{};
        try {NativeEffectTextureDictionary duplicate(game_root,dictionary);}
        catch(const std::logic_error&){duplicate_dictionary=true;}
        check(duplicate_dictionary && !system[11] && named.head==named_head && !named.count);
        NativeParticleEffectFactory objects(memory,documents,textures,address(system.data()),status,&lookup);
        NativeEffectObjectQueues queues(memory,g,objects,NativeEffectObjectQueues::BorrowInitialized{});
        queues.BindInlineTree(trees);
        NativeDialogParticlePlayback effects(memory,objects,queues);
        NativeEffectTaskFactory factory(memory,documents,tasks,objects,&lookup,&support);
        NativeDialogTaskRuntime scheduler(memory,documents,tasks,factory);
        NativeDialogResetRuntime reset(memory,tasks,effects);
        NativeDialogFrameRuntime frame(memory,tasks,reset,effects);
        NativePkFileManagerRuntime packages;
        std::array<std::uint32_t,20> filesystem{};filesystem[10]=0x1010u;
        packages.ProcessPackFiles(packages.InitializePackFileManager(),address(filesystem.data()));
        check(packages.LoadAllScripts()!=0u);
        const auto package_baseline=packages.LoadedPackageCount();
        NativeEffectDialogLoader loader(memory,documents,tasks,packages,address(system.data()),effects);
        check(!loader.Load("foot_2") && !loader.LiveDialogs() && !loader.CachedDocuments());
        NativeEffectObjectPool pool(memory,address(system.data()),loader,documents);
        NativeEffectSystemLifecycle system_lifecycle(memory,address(system.data()),factories,
            textures,named_owner,pool,loader,support);
        check(system_lifecycle.Enable(true));
        check(system[0]==0x10001u && system[22]==32u &&
            memory.ReadByte(system[36]+4u)==1u &&
            std::bit_cast<float>(memory.ReadDword(system[36]+8u))==1440000.0f);
        check(system[11] && named.head==named_head && !named.count);
        check(lifecycle.Enable(address(system.data())) && registry.Contains(g) && group[11]==0x3f800000u);
        check(!loader.Load("__missing_effect_dialog__") && !loader.CachedDocuments());
        check(packages.LoadedPackageCount()==package_baseline);
        const auto first=loader.Load("foot_2"),second=loader.Load("foot_2");
        check(first && second && first!=second && loader.LiveDialogs()==2u && loader.CachedDocuments()==1u);
        check(packages.LoadedPackageCount()==package_baseline);
        auto& first_dialog=loader.Dialog(first);auto& second_dialog=loader.Dialog(second);
        check(first_dialog.State().words[32]!=second_dialog.State().words[32]);
        loader.ClearCache();check(!loader.CachedDocuments());
        // Source cache can disappear while cloned configuration stays playable.
        auto& state=first_dialog.State();state.words[3]=address(group.data());
        NativeDialogParticleEffects activation(memory,first_dialog.Templates(),objects,queues);
        NativeDialogPlaybackRuntime playback(memory,tasks,reset,activation.ActivationRuntime(),scheduler,
            activation.CleanupRuntime(),effects);
        check(DialogObject::StartPlayback(state,playback));
        (void)DialogObject::UpdatePlayback(state,0.05f,frame);
        check(factory.LiveTasks()==3u && objects.LiveObjects()==1u && queues.Count(176u)==1u);
        loader.DestroyEffect(first);loader.DestroyEffect(second);
        check(!loader.LiveDialogs() && !objects.LiveObjects() && !factory.LiveTasks() && !queues.Count(176u));
        {
            pool.Enable(3u);
            const auto effect=pool.Acquire("foot_2");check(effect && loader.LiveDialogs()==1u);
            auto& dialog=loader.Dialog(effect);auto& s=dialog.State();s.words[3]=address(group.data());
            NativeDialogParticleEffects binding(memory,dialog.Templates(),objects,queues);
            NativeDialogPlaybackRuntime play(memory,tasks,reset,binding.ActivationRuntime(),scheduler,
                binding.CleanupRuntime(),effects);
            // The caller's handle configuration supplies NAME after Acquire's
            // original embedded-document cleanup, as required by ReturnEffect.
            documents.WriteStringAttribute(effect+20u,"NAME","foot_2");
            check(DialogObject::StartPlayback(s,play));
            (void)DialogObject::UpdatePlayback(s,0.05f,frame);
            const auto particle=memory.ReadDword(s.words[28]+16u);
            const auto resource=objects.Configuration(particle).State().words[23];
            (void)DialogObject::ResetRuntimeState(s,reset);
            pool.Return(effect,0u);
            check(!queues.Count(176u) && !factory.LiveTasks());
            check(pool.Acquire("foot_2")==effect);
            check(DialogObject::StartPlayback(s,play));
            check(memory.ReadDword(s.words[28]+16u)==particle &&
                objects.Configuration(particle).State().words[23]==resource);
            struct Events final : INativeEffectPlaybackEvents {
                NativeArenaEffectServices* native{};
                std::uint32_t group{},id{17u};unsigned starts{},finishes{},removals{};
                void Started(std::uint32_t g,const EffectSystemHandle& h,std::int32_t) override {
                    if(g!=group || h.system!=g || h.id!=id) throw std::runtime_error("native effect start handle");
                    native->Started(g,h,1);
                    ++starts;
                }
                void Finished(std::uint32_t g,const EffectSystemHandle& h,std::int32_t) override {
                    if(g!=group || h.system!=g || h.id!=id) throw std::runtime_error("native effect finish handle");
                    native->Finished(g,h,1);
                    ++finishes;
                }
                void Removed(std::uint32_t g,const EffectSystemHandle& h,std::uint32_t) override {
                    if(g!=group || h.system!=g || h.id!=id) throw std::runtime_error("native effect remove handle");
                    native->Removed(g,h,0u);
                    ++removals;
                }
            } events;events.group=g;events.native=&arena_services;
            NativeEffectActiveFrame active(memory,documents,tasks,g,trees,loader,pool,objects,queues,
                effects,scheduler,events);
            NativeDialogStopRuntime nested_stop(memory,tasks,effects);
            NativeNestedEffectDialogs nested(loader,active,nested_stop);
            objects.BindNestedDialogs(nested,queues);
            struct UnbindNested {
                NativeParticleEffectFactory& objects;
                ~UnbindNested(){objects.UnbindNestedDialogs();}
            } unbind_nested{objects};
            lifecycle_services.active=&active;
            struct StopGroupBeforePlaybackServices {
                NativeEffectGroupLifecycle& lifecycle;
                ~StopGroupBeforePlaybackServices(){lifecycle.Disable();}
            } stop_group{lifecycle};
            // Original creation starts the prepared Dialog before registering
            // its ongoing frame work. The legacy Pause entry enables tasks.
            active.PausePlayback(effect);
            NativeParticleGroupFrames frames(memory,objects,0u,arena_services);
            NativeEffectGroupFrame group_frame(memory,g,trees,active,frames);
            documents.WriteStringAttribute(effect+20u,"NAME","foot_2");
            const auto entry=trees.InsertActive(g+12u,17u,effect,2);check(entry.inserted);
            group[1]=0u;group_frame.Update(10.0);
            check(memory.ReadDword(entry.node+20u)==2u && events.starts==0u && events.finishes==0u);
            group[1]=1u;
            bool drawn{};
            for(unsigned tick=0u;tick<100u && group[6];++tick) {
                group_frame.Update(0.05);
                if(tick==6u) {
                    NativeParticleProbeCamera camera;camera.Begin();
                    auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
                    check(device!=nullptr);
                    check(SUCCEEDED(device->SetRenderState(D3DRS_FOGENABLE,1u)));
                    group_frame.Draw(0u);
                    DWORD fog{};check(SUCCEEDED(device->GetRenderState(D3DRS_FOGENABLE,&fog)) && fog==1u);
                    drawn=camera.EndPixels()>0u;
                    check(SUCCEEDED(device->SetRenderState(D3DRS_FOGENABLE,0u)));
                }
            }
            check(drawn);check(!group[6]);check(events.starts==1u);check(events.finishes==1u);
            check(!factory.LiveTasks() && !queues.Count(176u) && pool.Acquire("foot_2")==effect);
            // Infinite mode starts an idle Dialog and writes its ID. Preseed
            // a different value so the native typed setter branch runs too.
            documents.WriteStringAttribute(effect+20u,"NAME","foot_2");
            documents.WriteIntAttribute(effect+20u,"ID",99);
            const auto infinite=trees.InsertActive(g+12u,17u,effect,-1);check(infinite.inserted);
            const auto start_count=events.starts;
            for(unsigned tick=0u;tick!=100u;++tick) group_frame.Update(0.05);
            check(events.starts>=start_count+2u && group[6]==1u);
            check(documents.AttributeToInt(documents.FindNumericAttribute(effect+20u,"ID"),-1)==17);
            active.ResetDialog(effect);pool.Return(effect,0u);
            (void)trees.EraseActive(g+12u,infinite.node);
            check(!group[6] && pool.Acquire("foot_2")==effect);
            // Retire a real live Dialog through the existing handle path:
            // stop tasks now, drain its particle duration, then cache it.
            {
                NativeDialogStopRuntime stop(memory,tasks,effects);
                NativePlayerTransformRuntime transforms;
                struct Owner final : INativeEffectHandleOwnerServices {
                    NativeEffectActiveFrame& active;NativeEffectObjectPool& pool;
                    std::uint32_t group,system;unsigned releases{},resets{},returns{};
                    Owner(NativeEffectActiveFrame& a,NativeEffectObjectPool& p,
                          std::uint32_t g,std::uint32_t s):active(a),pool(p),group(g),system(s){}
                    std::string ReadNameAttribute(std::uint32_t) override {
                        throw std::logic_error("content retirement does not read Name");
                    }
                    void ResetDialogRuntimeState(std::uint32_t d) override {active.ResetDialog(d);++resets;}
                    void DispatchOwnerRelease(std::uint32_t g,EffectSystemHandleWords h,std::uint32_t mode) override {
                        if(g!=group || h.system!=g || h.id!=17u || mode)
                            throw std::runtime_error("live particle owner release");
                        ++releases;
                    }
                    void ReturnEffect(std::uint32_t p,std::uint32_t d,std::uint32_t mode) override {
                        if(p!=system+84u || mode || resets!=returns+1u)
                            throw std::runtime_error("live pending reset/return ordering");
                        pool.Return(d,0u);++returns;
                    }
                    void ProcessEffectList(std::uint32_t) override {
                        throw std::logic_error("content retirement uses pending timers");
                    }
                } host(active,pool,g,address(system.data()));
                NativeEffectHandleContentRuntime content(memory,address(system.data()),registry.Nil(),
                    trees.ActiveNil(),trees.PendingNil(),stop,transforms,host);
                VerifyNativeEffectContextHandles(memory,trees,g,content);
                VerifyNativeEffectXml(game_root,memory,documents,textures,trees,packages,g,content,active,group_frame);
                active.StartPlayback(effect);
                active.PausePlayback(effect);
                documents.WriteStringAttribute(effect+20u,"NAME","foot_2");
                check(trees.InsertActive(g+12u,17u,effect,1).inserted);
                for(unsigned tick=0u;tick<3u;++tick) group_frame.Update(0.05);
                EffectSystemHandleWords handle{17u,g};
                const auto duration=objects.Duration(particle);
                check(duration>0.0 && factory.LiveTasks()==3u);
                EffectHandle::UpdateContent(handle,content);
                check(!handle.system && !group[6] && group[10]==1u && host.releases==1u);
                check(!factory.LiveTasks() && !s.words[2] && queues.Count(176u)==1u &&
                    memory.ReadByte(particle+11u)==1u);
                const auto pending=memory.ReadDword(group[8]);
                const auto remaining=content.ReadFloat(pending+16u);
                check(remaining==static_cast<float>(duration));
                group[1]=0u;group_frame.Update(10.0);
                check(content.ReadFloat(pending+16u)==remaining && !host.returns);
                group[1]=1u;
                // Just beyond the exact double boundary, but below a float
                // ULP: original rounding must produce exactly zero remaining.
                group[11]=std::bit_cast<std::uint32_t>(0.5f);
                group_frame.Update(2.0*duration+1.0e-9);
                group[11]=0x3f800000u;
                // The original tests expiry before subtracting the current
                // delta: zero is returned on the next timer traversal.
                check(group[10]==1u && !host.returns && content.ReadFloat(pending+16u)==0.0f);
                group_frame.Update(0.0);
                check(!group[10] && !queues.Count(176u));
                group_frame.Update(1.0);check(!group[10] && !queues.Count(176u));
                check(pool.Acquire("foot_2")==effect);
                active.StartPlayback(effect);active.PausePlayback(effect);
                check(objects.Configuration(particle).State().words[23]==resource && factory.LiveTasks()==3u);
                // Leave one stopped Dialog in pending storage while another
                // Dialog is active, so group cleanup covers both ownership paths.
                documents.WriteStringAttribute(effect+20u,"NAME","foot_2");
                check(trees.InsertActive(g+12u,17u,effect,1).inserted);
                handle={17u,g};EffectHandle::UpdateContent(handle,content);
                check(group[10]==1u && !group[6]);
                const auto extra=pool.Acquire("foot_2");check(extra && extra!=effect);
                loader.Dialog(extra).State().words[3]=g;
                active.StartPlayback(extra);active.PausePlayback(extra);
                documents.WriteStringAttribute(extra+20u,"NAME","foot_2");
                check(trees.InsertActive(g+12u,17u,extra,1).inserted);
                const auto drain_node=memory.ReadDword(group[8]);
                const auto before=content.ReadFloat(drain_node+16u);
                constexpr double delta=0.123456789;
                group[11]=std::bit_cast<std::uint32_t>(0.5f);
                group_frame.Update(delta);
                const auto expected=static_cast<float>(static_cast<double>(before)-
                    static_cast<double>(static_cast<float>(0.5*delta)));
                check(content.ReadFloat(drain_node+16u)==expected && group[10]==1u && group[6]==1u);
                // Zero scale consumes no further lifetime on this positive
                // pending record. Ready/expired records can still be retired.
                group[11]=0u;group_frame.Update(1.0);
                check(content.ReadFloat(drain_node+16u)==expected && group[6]==1u);
                group[11]=0x3f800000u;
            }
            // Disable while the real Dialog/tasks/particles are live. Shared
            // cleanup must reset/return them before clearing enabled/registry.
            lifecycle.Disable();
            check(!group[1] && !group[12] && !registry.Contains(g) && !group[6] && !group[10]);
            check(!factory.LiveTasks() && !queues.Count(176u) && events.removals==1u);
            check(lifecycle_services.cleanups==1u);
            lifecycle.Disable();check(lifecycle_services.cleanups==1u);
            check(lifecycle.Enable(address(system.data())) && registry.Contains(g));
            VerifyNativeNestedEffectObjects(game_root,memory,documents,objects,queues,loader,
                group_frame,factory,tasks,packages,effect,g,address(system.data()),arena_services,support);
            struct CreationEvents final : INativeEffectCreationEvents {
                NativeArenaEffectServices* native{};
                NativeBaseObjectMemory& memory;NativeEventDocumentRuntime& documents;
                std::uint32_t group;unsigned calls{};bool reject{};
                CreationEvents(NativeBaseObjectMemory& m,NativeEventDocumentRuntime& d,std::uint32_t g)
                    :memory(m),documents(d),group(g){}
                void Created(std::uint32_t g,const EffectSystemHandle& h,
                    const std::array<float,3>& p,std::uint32_t repeats) override {
                    if(g!=group || h.system!=g || repeats!=1u || p!=std::array{3.0f,4.0f,5.0f})
                        throw std::runtime_error("native creation event payload");
                    const auto node=memory.ReadDword(memory.ReadDword(g+16u));
                    const auto dialog=memory.ReadDword(node+16u);
                    if(memory.ReadDword(node+12u)!=h.id ||
                        documents.AttributeToInt(documents.FindNumericAttribute(dialog+20u,"ID"),-1)!=static_cast<int>(h.id))
                        throw std::runtime_error("native creation event sees registered ID");
                    ++calls;
                    native->Created(g,h,p,repeats);
                    if(reject) throw std::runtime_error("test creation event rejection");
                }
            } created(memory,documents,g);
            created.native=&arena_services;
            NativeEffectCreation creation(memory,documents,packages,address(system.data()),g,
                loader,pool,trees,active,created);
            group[2]=17u;
            const auto live_before=loader.LiveDialogs();
            const auto cached_before=system[26];
            const auto created_handle=creation.Create("foot_2",{3.0f,4.0f,5.0f},1,1u);
            check(created_handle.system==g && created_handle.id==17u && group[2]==18u && created.calls==1u);
            check(loader.LiveDialogs()==live_before+1u);
            const auto created_node=memory.ReadDword(group[4]);
            const auto created_dialog=memory.ReadDword(created_node+16u);
            check(memory.ReadByte(created_dialog+81u)==1u && memory.ReadByte(created_dialog+83u)==1u);
            check(loader.ReadEffectName(created_dialog+20u)=="FOOT_2");
            auto& created_state=loader.Dialog(created_dialog).State();
            check(created_state.words[44]==std::bit_cast<std::uint32_t>(3.0f) &&
                created_state.words[45]==std::bit_cast<std::uint32_t>(4.0f) &&
                created_state.words[46]==std::bit_cast<std::uint32_t>(5.0f));
            for(unsigned tick=0u;tick<100u && group[6];++tick) group_frame.Update(0.05);
            // Uppercase creation uses a distinct cache key; Return may trim an
            // older entry at the configured three-entry capacity.
            const auto trimmed=cached_before+1u>=3u?1u:0u;
            check(!group[6] && !factory.LiveTasks() && loader.LiveDialogs()==live_before+1u-trimmed);
            check(packages.LoadedPackageCount()==package_baseline);
            // Exercise original singleton+2 normalization and a cold document
            // load under nested creation/file-loader Effect package scopes.
            loader.ClearCache();
            memory.WriteByte(address(system.data())+2u,1u);
            events.id=group[2];
            const auto upper=creation.Create("foot_2",{3.0f,4.0f,5.0f});
            check(upper.system==g && upper.id==events.id && loader.CachedDocuments()==1u);
            const auto upper_dialog=memory.ReadDword(memory.ReadDword(group[4])+16u);
            check(loader.ReadEffectName(upper_dialog+20u)=="FOOT_2");
            check(packages.LoadedPackageCount()==package_baseline);
            for(unsigned tick=0u;tick<100u && group[6];++tick) group_frame.Update(0.05);
            check(!group[6] && !factory.LiveTasks());
            memory.WriteByte(address(system.data())+2u,0u);
            // Execute the real generic script callback into original EFT
            // loading, native creation, task scheduling, GPU drawing and return.
            {
                NativeScriptEffectRuntime script_effects(memory,creation);
                NativeGameScriptServices services{};services.effects=&script_effects;
                NativeGameScriptCallbacks callbacks(services);
                NativeAngelScriptEngine engine;engine.SetCallbacks(&callbacks);
                constexpr char source[]=
                    "void Spawn() { string name = \"foot_2\"; "
                    "giEffectPlay(name, 1, 3.0f, 4.0f, 5.0f); name = \"changed\"; } "
                    "void Missing() { giEffectPlay(\"__missing_effect__\", 1, 3, 4, 5); }";
                check(engine.Raw()->AddScriptSection("effect_probe","effect_probe.cs",source,sizeof(source)-1)>=0);
                check(engine.Raw()->Build("effect_probe")>=0);
                struct Context {
                    asIScriptContext* value{};
                    ~Context(){if(value)value->Release();}
                } context;
                check(engine.Raw()->CreateContext(&context.value)>=0 && context.value);
                const auto run=[&](const char* function) {
                    const auto id=engine.FunctionId("effect_probe",function);
                    check(id>=0 && context.value->Prepare(id)>=0);
                    check(context.value->Execute()==asEXECUTION_FINISHED);
                };
                events.id=group[2];const auto notifications=created.calls;
                run("Spawn");
                check(group[6]==1u && group[2]==events.id+1u && created.calls==notifications+1u);
                const auto script_dialog=memory.ReadDword(memory.ReadDword(group[4])+16u);
                check(loader.ReadEffectName(script_dialog+20u)=="foot_2" && factory.LiveTasks()==3u);
                // Reset invalidates only the temporary id/owner pair. The
                // active effect survives until ordinary frame completion.
                ScriptEffectHandle temporary{events.id,g};
                (void)script_effects.ResetHandle(temporary);
                check(temporary.first==0xffffffffu && !temporary.second && group[6]==1u);
                bool script_pixels{};
                for(unsigned tick=0u;tick<100u && group[6];++tick) {
                    group_frame.Update(0.05);
                    if(tick==6u) {
                        NativeParticleProbeCamera camera;camera.Begin();group_frame.Draw(0u);
                        script_pixels=camera.EndPixels()>0u;
                    }
                }
                check(script_pixels && !group[6] && !factory.LiveTasks() && !queues.Count(176u));
                bool no_actors{};
                try {(void)script_effects.PlayerManager();}
                catch(const std::logic_error&){no_actors=true;}
                check(no_actors);
                const std::array actor_position{3.0f,4.0f,5.0f};
                events.id=group[2];
                auto positioned=script_effects.CreateAtAddress(g,address("foot_2"),
                    address(actor_position.data()),1u,0u);
                check(positioned.first==events.id && positioned.second==g && group[6]==1u);
                (void)script_effects.ResetHandle(positioned);
                check(group[6]==1u);
                for(unsigned tick=0u;tick<100u && group[6];++tick) group_frame.Update(0.05);
                check(!group[6] && !factory.LiveTasks() && !queues.Count(176u));
                const auto missing_id=group[2];run("Missing");
                check(group[2]==missing_id && !group[6] && created.calls==notifications+2u);
                check(packages.LoadedPackageCount()==package_baseline);
            }
            const auto next_id=group[2];
            check(!creation.Create("__missing_effect__",{3.0f,4.0f,5.0f}).system && group[2]==next_id && !group[6]);
            check(packages.LoadedPackageCount()==package_baseline);
            created.reject=true;
            bool rejected{};
            try {(void)creation.Create("foot_2",{3.0f,4.0f,5.0f});}
            catch(const std::runtime_error&){rejected=true;}
            check(rejected && !group[6] && !factory.LiveTasks() && !queues.Count(176u));
            check(packages.LoadedPackageCount()==package_baseline);
            created.reject=false;
            const auto cleanups_before_shutdown=lifecycle_services.cleanups;
            group[74]=0x12345678u;
            manager.ShutdownEffects();
            check(!memory.ReadByte(g+4u) && !registry.Contains(g) && !group[74] &&
                arena_shared.script_callback==0x12345678u && group[71] && arena_shared.instance_count==1u);
            manager.ShutdownEffects();
            check(arena_shared.script_callback==0x12345678u && !group[74]);
            const auto shutdown_cleanups=lifecycle_services.cleanups-cleanups_before_shutdown;
            check(shutdown_cleanups==3u); // explicit cleanup twice; enabled Disable also cleans once.
            check(lifecycle.Enable(address(system.data())) && registry.Contains(g));
            const auto deferred=creation.CreateWithoutPause("foot_2",{3.0f,4.0f,5.0f});
            check(deferred.system==g && deferred.id==group[2]-1u);
            events.id=deferred.id;
            const auto deferred_dialog=memory.ReadDword(memory.ReadDword(group[4])+16u);
            check(memory.ReadByte(deferred_dialog+80u)==1u && !memory.ReadByte(deferred_dialog+81u));
            group_frame.Update(1.0);check(group[6]==1u);
            check(active.EnableDialogTasks(deferred_dialog));
            memory.WriteByte(g+260u,1u); // enabled but empty CG dispatcher
            manager.Destroy();manager.Destroy();
            check(scripts.visits==1u && scripts.saw_live_group && scripts.saw_script_disabled &&
                !memory.ReadByte(g+260u));
            check(!arena_shared.instance_count && !arena_shared.sentinel && !group[71] && !group[73]);
            check(events.removals==2u && lifecycle_services.cleanups==2u+shutdown_cleanups);
            check(!registry.Contains(g) && !group[1] && !factory.LiveTasks());
            for(unsigned offset=68u;offset<=200u;offset+=12u)
                check(!memory.ReadDword(g+offset+4u) && !memory.ReadDword(g+offset+8u));
            system_lifecycle.Cleanup();
            check(!loader.LiveDialogs() && !objects.LiveObjects() && !factory.LiveTasks());
            lifecycle_services.active=nullptr;
            arena_services.UnbindPlayback();
        }
        check(!memory.ReadByte(address(system.data())) && !loader.CachedDocuments() &&
            !system[5] && !system[9] && !system[11] && !memory.ReadByte(system[36]+4u));
        check(!loader.Load("foot_2"));
        system_lifecycle.Cleanup();
        // Re-enable through the original full lifecycle and acquire fresh
        // resources after the old group and its queues were destroyed.
        check(system_lifecycle.Enable(false));
        check(system[0]==0x10001u && system[5]==24u && system[9]==11u &&
            !memory.ReadByte(system[36]+4u));
        const auto replay=pool.Acquire("foot_2");
        check(replay && loader.CachedDocuments()==1u);
        documents.WriteStringAttribute(replay+20u,"NAME","foot_2");
        pool.Return(replay,0u);
        check(pool.Acquire("foot_2")==replay);
        pool.Return(replay,0u);
        // Enable on an already enabled system must drain cached payloads and
        // register fresh factories, rather than leaking the previous cycle.
        check(system_lifecycle.Enable(true));
        check(!loader.LiveDialogs() && !loader.CachedDocuments() && system[5]==24u && system[9]==11u);
        system_lifecycle.Cleanup();
        check(!loader.CachedDocuments() && !loader.LiveDialogs() && !system[5] && !system[9]);
        check(packages.LoadedPackageCount()==package_baseline);
        manager.Destroy();manager.Destroy();
        check(!system[11] && system[12]==1u && named.head==named_head && !named.count);
        check(lookup.object_calls && lookup.task_calls);
        check(!trees.ActiveNil() && !trees.PendingNil() && !trees.InlineNil() &&
            !trees.ContextNil() && !trees.TimedNil());
        std::cout<<"Native effect Dialog loader: original PK effect scope, cold/cache loads, independent Dialogs, "
            "source-cache cleanup, actual particle pool replay, finite/infinite group frames, live handle stop/drain/return and active teardown verified\n";
    }
    check(!tasks.RegisteredTasks() && !documents.DocumentCount() && !memory.OutstandingAllocations() &&
        !support_globals.random && !support_globals.auxiliary);
}
}
