#pragma once
#include "pal4/native_dialog_particle_effects.h"
#include "pal4/native_particle_effect_factory.h"
#include "pal4/native_effect_texture_dictionary.h"
#include "pal4/native_dialog_object.h"
#include "pal4/native_effect_text_manager.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_package_archive.h"
#include "pal4/binary_xml_config.h"
#include <rwcore.h>
#include <array>
#include <filesystem>
#include <iostream>
#include <source_location>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeDialogParticleActivation(const std::filesystem::path& game_root) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) throw std::runtime_error("native Dialog particle activation line "+std::to_string(at.line()));
    };
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    const auto children=[](RwFrame* frame) {
        unsigned count{};
        (void)RwFrameForAllChildren(frame,[](RwFrame* f,void* p)->RwFrame* {++*static_cast<unsigned*>(p);return f;},&count);
        return count;
    };
    NativeBaseObjectMemory memory;NativeEventDocumentRuntime documents(memory);
    std::array<std::uint32_t,15> storage{};const auto root=address(storage.data());
    (void)documents.InitializeDocument(root,"activation",0u);
    {
        struct Status final : INativeEffectTaskStatus {
            bool enabled=true,active=false;
            bool EffectsEnabled() override{return enabled;}
            bool EffectIsActive(std::uint32_t) override{return active;}
        } status;
        std::array<std::uint32_t,42> system{};std::array<std::uint32_t,53> context{};context[1]=1u;
        NativeEffectTextureDictionary textures(game_root);
        NativeParticleEffectFactory objects(memory,documents,textures,address(system.data()),status);
        NativeEffectObjectQueues queues(memory,address(context.data()),objects);
        NativeDialogObject dialog(memory,documents);
        NativeDialogParticleEffects activation(memory,dialog.Templates(),objects,queues);
        NativePackageArchive archive;check(archive.Open((game_root/"gamedata/Effect.cpk").string().c_str()));
        const auto data=archive.ReadMember("script\\foot_2.eft");BinaryXmlConfig parsed;check(data && parsed.Parse(*data));
        // Keep the real first particle. The second reference fails only after
        // the first has been constructed, queued and attached.
        auto config=parsed.Root();
        for(auto& child:config.children) if(child.name=="ObjectList")
            child.children.push_back({"Object",{{"ObjectId",3u,std::string("second")}}, {}});
        const auto bytes=BinaryXmlConfig::Serialize(config);check(bytes.has_value());
        check(documents.LoadBinaryDocument(root,*bytes) && dialog.Load(root));documents.CleanupDocument(root);
        auto& state=dialog.State();state.words[3]=address(context.data());
        auto* frame=RwFrameCreate();check(frame);state.words[43]=address(frame);
        const auto baseline=memory.OutstandingAllocations();
        check(!activation.Activate(state,dialog.Address()));
        check(!objects.LiveObjects() && !system[41] && !queues.Count(176u) &&
            !children(frame) && memory.OutstandingAllocations()==baseline);
        const auto first=dialog.Templates().Find("lz.1");check(first && dialog.Templates().Add("second",first));
        const auto second=dialog.Templates().Find("second");check(second);
        documents.WriteStringAttribute(second,"Attr_Class_Name","ClumpObject");
        const auto with_second=memory.OutstandingAllocations();
        bool rejected=false;try{(void)activation.Activate(state,dialog.Address());}catch(const std::logic_error&){rejected=true;}
        check(rejected && !objects.LiveObjects() && !system[41] && !queues.Count(176u) && !children(frame) &&
            memory.OutstandingAllocations()==with_second);
        documents.WriteStringAttribute(second,"Attr_Class_Name","RtkPrtStdAtomicObject");
        check(activation.Activate(state,dialog.Address()) && objects.LiveObjects()==2u && system[41]==2u && queues.Count(176u)==2u);
        rejected=false;try{(void)activation.Activate(state,dialog.Address());}catch(const std::logic_error&){rejected=true;}
        check(rejected && objects.LiveObjects()==2u && queues.Count(176u)==2u);
        activation.Stop(state);activation.Stop(state);
        check(!objects.LiveObjects() && !system[41] && !queues.Count(176u) && !children(frame));
        // Original disabled+active policy creates payloads while skipping queue
        // and frame association; cleanup still destroys the payloads.
        status.enabled=false;status.active=true;
        check(activation.Activate(state,dialog.Address()) && objects.LiveObjects()==2u &&
            !queues.Count(176u) && !children(frame));
        activation.Stop(state);check(!objects.LiveObjects() && !system[41]);
        for(auto p=state.words[28];p!=state.words[29];p+=20u) check(!memory.ReadDword(p+16u));
        check(RwFrameDestroy(frame));state.words[43]=0u;
        rejected=false;try{(void)activation.Activate(state,dialog.Address());}catch(const std::logic_error&){rejected=true;}
        check(rejected && !objects.LiveObjects());
        std::cout << "Native Dialog particle activation: templates/create/queue/frame association; "
                     "partial failure rollback, retry, disabled policy and cleanup verified\n";
    }
    documents.DestroyDocument(root);check(!documents.DocumentCount() && !memory.OutstandingAllocations());
}
}
