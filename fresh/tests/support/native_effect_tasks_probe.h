#pragma once
#include "pal4/native_effect_task_factory.h"
#include "pal4/native_dialog_object.h"
#include "pal4/native_dialog_properties.h"
#include "pal4/native_dialog_pre_delete_runtime.h"
#include "pal4/native_dialog_config_runtime.h"
#include "pal4/native_effect_text_manager.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_script_object_runtime.h"
#include "pal4/native_package_archive.h"
#include "pal4/binary_xml_config.h"
#include "pal4/rtk_prt_std_atomic_object.h"
#include "pal4/rtk_prt_std_atomic_control.h"
#include "pal4/rtk_prt_std_atomic_task_facets.h"
#include "pal4/rtk_prt_std_atomic_task_slot_resolver.h"
#include "pal4/rtk_prt_std_atomic_properties.h"
#include <bit>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <rwcore.h>

namespace pal4::test {
inline void VerifyNativeEffectTasks(const std::filesystem::path& game_root) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) {
            std::cerr << "native effect tasks line " << at.line() << std::endl;
            throw std::runtime_error("native effect tasks line "+std::to_string(at.line()));
        }
    };
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    NativeBaseObjectMemory memory;
    NativeEventDocumentRuntime documents(memory);
    NativeScriptObjectRuntime tasks(memory);
    struct Targets final : INativeEffectTaskFacets,IRtkPrtStdAtomicObjectRuntime,
        IRtkPrtStdAtomicTaskFacetRuntime,IRtkPrtStdAtomicPropertiesRuntime,
        INativeDialogDeleteEffects {
        NativeBaseObjectMemory& memory;
        std::array<std::uint32_t,44> system{};
        RtkPrtStdAtomicObjectState object{};
        std::array<std::uint32_t,80> resource{};
        bool enabled=true;
        bool active=false;
        RwFrame* effect_frame{};
        unsigned modes{},references{},events{},destroyed{};
        bool parent_at_destroy{};
        unsigned shown{},hidden{},stopped{},emitted{};
        unsigned frame_updates{};
        explicit Targets(NativeBaseObjectMemory& m):memory(m) {
            (void)RtkPrtStdAtomicObject::Construct(object,*this);
            object.words[1]=1u;
            object.words[23]=reinterpret_cast<std::uint32_t>(resource.data());
        }
        ~Targets() override {RtkPrtStdAtomicObject::Destroy(object,*this);}
        std::uint32_t Owner() const {return reinterpret_cast<std::uint32_t>(&object);}
        std::uint32_t SystemInstance() override {return reinterpret_cast<std::uint32_t>(system.data());}
        std::uint32_t ReadWord(std::uint32_t a) override {return memory.ReadDword(a);}
        void WriteWord(std::uint32_t a,std::uint32_t v) override {memory.WriteDword(a,v);}
        float ReadFloat(std::uint32_t a) override {return std::bit_cast<float>(ReadWord(a));}
        std::uint8_t ReadByte(std::uint32_t a) override {return memory.ReadByte(a);}
        void WriteByte(std::uint32_t a,std::uint8_t v) override {memory.WriteByte(a,v);}
        void WriteFloat(std::uint32_t a,float v) override {WriteWord(a,std::bit_cast<std::uint32_t>(v));}
        std::uint8_t UpdateFrame(std::uint32_t p) override {
            if(p!=addressResource()) throw std::invalid_argument("particle resource refresh");
            ++frame_updates;return 1u;
        }
        std::uint32_t addressResource() const {return reinterpret_cast<std::uint32_t>(resource.data());}
        std::int32_t FloatToInt(double v) override {
            if(!(v>=std::numeric_limits<std::int32_t>::min() && v<=std::numeric_limits<std::int32_t>::max()))
                throw std::out_of_range("particle emission count");
            return static_cast<std::int32_t>(v);
        }
        bool EffectsEnabled() override {return enabled;}
        void PlayEffect(std::uint32_t,std::uint8_t) override {throw std::logic_error("particle-only fixture has no play facet");}
        void SetColorFacetEnabled(std::uint32_t,std::uint8_t) override {throw std::logic_error("particle-only fixture has no color facet");}
        void ReadColor(std::uint32_t,std::array<std::uint8_t,4>&) override {throw std::logic_error("particle-only fixture has no color facet");}
        void SetColor(std::uint32_t,const std::array<std::uint8_t,4>&) override {throw std::logic_error("particle-only fixture has no color facet");}
        bool EffectIsActive(std::uint32_t effect) override {
            if(effect!=Owner()) throw std::invalid_argument("particle effect handle");
            return active;
        }
        std::uint32_t ResolveFacet(std::uint32_t effect,std::string_view task) override {
            if(effect!=Owner()) throw std::invalid_argument("particle task owner");
            std::uint32_t facet{};
            (void)RtkPrtStdAtomicTaskSlotResolver::Resolve(effect,task,facet);return facet;
        }
        void ValidateFacet(std::uint32_t facet,std::uint32_t offset) {
            if(facet!=Owner()+offset) throw std::invalid_argument("particle task facet");
        }
        void Show(std::uint32_t facet) override {
            ValidateFacet(facet,20u);(void)RtkPrtStdAtomicControl::EnablePlayback(object);++shown;
        }
        void Hide(std::uint32_t facet) override {
            ValidateFacet(facet,20u);(void)RtkPrtStdAtomicControl::DisablePlayback(object);++hidden;
        }
        void StopEmission(std::uint32_t facet) override {
            ValidateFacet(facet,48u);(void)RtkPrtStdAtomicTaskFacets::DisableResourceFlag(object,*this);++stopped;
        }
        void StartEmission(std::uint32_t facet,float time,std::uint8_t increase) override {
            ValidateFacet(facet,48u);(void)RtkPrtStdAtomicTaskFacets::SetEmissionTime(object,time,increase,0u,*this);++emitted;
        }
        void ReadScale(std::uint32_t facet,std::array<float,3>& v) override {
            ValidateFacet(facet,28u);std::array<std::uint32_t,3> bits{};
            (void)RtkPrtStdAtomicProperties::GetScale(object,bits,*this);
            for(unsigned i=0;i<3;++i) v[i]=std::bit_cast<float>(bits[i]);
        }
        void SetScale(std::uint32_t facet,const std::array<float,3>& v,std::uint32_t mode) override {
            ValidateFacet(facet,28u);
            if(mode>1u) throw std::invalid_argument("particle scale mode");
            (void)RtkPrtStdAtomicProperties::SetScale(object,v,mode!=0u,*this);
        }
        void SetEffectMode(std::uint32_t e,std::uint32_t mode) override {
            if(e!=Owner() || mode) throw std::invalid_argument("particle cleanup mode");
            ++modes;
        }
        void ReleaseEffectReferences(std::uint32_t e) override {
            if(e!=Owner()) throw std::invalid_argument("particle cleanup references");
            ++references;
        }
        void ProcessEffectEvent(std::uint32_t owner,std::uint32_t e) override {
            if(owner!=7u || e!=Owner()) throw std::invalid_argument("particle cleanup owner event");
            ++events;
        }
        std::uint32_t EffectFrame(std::uint32_t e) override {
            if(e!=Owner()) throw std::invalid_argument("particle cleanup frame");
            return reinterpret_cast<std::uint32_t>(effect_frame);
        }
        void DestroyEffect(std::uint32_t e) override {
            if(e!=Owner() || !effect_frame) throw std::invalid_argument("particle cleanup destruction");
            parent_at_destroy=RwFrameGetParent(effect_frame)!=nullptr;
            if(parent_at_destroy && !RwFrameRemoveChild(effect_frame)) throw std::runtime_error("fixture effect detach");
            if(!RwFrameDestroy(effect_frame)) throw std::runtime_error("fixture effect destruction");
            effect_frame=nullptr;++destroyed;
        }
    } targets(memory);
    check(targets.system[41]==1u);
    NativeEffectTaskFactory factory(memory,documents,tasks,targets);
    NativeDialogTaskRuntime task_runtime(memory,documents,tasks,factory);
    std::array<std::uint32_t,15> storage{};
    const auto root=address(storage.data());
    (void)documents.InitializeDocument(root,"effect",0u);
    {
        NativeDialogObject owner(memory,documents);
        (void)documents.CreateChild(root,"OxnList");
        const auto list=documents.CreateChild(root,"TargetList");
        for(const auto name:{"z","T1","a"}) {
            const auto n=documents.CreateChild(list,"Target");
            documents.WriteStringAttribute(n,"TargetName",name);
            documents.WriteFloatAttribute(n,"TargetPosX",1);
            documents.WriteFloatAttribute(n,"TargetPosY",2);
            documents.WriteFloatAttribute(n,"TargetPosZ",3);
        }
        check(owner.Load(root));documents.CleanupDocument(root);
        auto& properties=owner.Properties();
        std::array<float,3> out{9,9,9};
        check(properties.GetStoredVector("T1",out) && out==std::array<float,3>{1,2,3});
        check(!properties.GetStoredVector("b",out) && out==std::array<float,3>{1,2,3});
        properties.SetFacet("T1",0u);check(!properties.FacetCount());
        std::array<float,3> live{10,20,30},replacement{40,50,60};
        std::string key="T1";properties.SetFacet(key,address(live.data()));key="changed";
        const NativeDialogProperties::Reader reader=[](std::uint32_t p) {
            return *reinterpret_cast<const std::array<float,3>*>(p);
        };
        check(properties.GetVector("T1",out,reader) && out==live);
        live[0]=11;check(properties.GetVector("T1",out,reader) && out==live);
        properties.SetFacet("T1",0u);check(properties.GetVector("T1",out,reader) && out==live);
        properties.SetFacet("T1",address(replacement.data()));check(properties.FacetCount()==1u);
        check(properties.GetVector("T1",out,reader) && out==replacement);
        check(properties.GetStoredVector("T1",out) && out==std::array<float,3>{1,2,3});
        properties.SetFacet("a",address(live.data()));properties.SetFacet("z",address(live.data()));
        check(properties.GetVector("a",out,reader) && out==live);
        check(!properties.GetVector("b",out,reader) && !properties.GetVector("zz",out,reader));
        bool rejected=false;try{(void)properties.GetVector("T1",out,{});}catch(const std::logic_error&){rejected=true;}
        check(rejected);
        owner.ClearFacets();check(!properties.FacetCount());
        check(properties.GetVector("T1",out,{}) && out==std::array<float,3>{1,2,3});
        properties.SetFacet("T1",address(live.data()));owner.Clear();
        check(!properties.FacetCount() && !properties.GetStoredVector("T1",out));
        check(live==std::array<float,3>{11,20,30} && replacement==std::array<float,3>{40,50,60});
        std::cout<<"Native Dialog properties: modern owned keys, borrowed live facets, replacement/null rules, target fallback and cleanup verified\n";
    }
    {
        NativeDialogObject owner(memory,documents);
        auto& state=owner.State();
        check(owner.Address()==address(&state) && state.words[0]==DialogObject::kVtable &&
            state.words[4]==owner.Templates().Address() && documents.DocumentCount()==2u);
        {
            NativeDialogObject second(memory,documents);
            check(owner.FacetNil()==second.FacetNil() && owner.TargetNil()==second.TargetNil() &&
                state.words[36]!=second.State().words[36] && state.words[40]!=second.State().words[40]);
            NativeBaseObjectMemory other_memory;
            NativeEventDocumentRuntime other_documents(other_memory);
            bool rejected=false;
            try {NativeDialogObject other(other_memory,other_documents);}
            catch(const std::invalid_argument&) {rejected=true;}
            check(rejected && !other_memory.OutstandingAllocations());
        }
        NativePackageArchive archive;
        const auto path=(game_root/"gamedata"/"Effect.cpk").string();
        check(archive.Open(path.c_str()));
        const auto data=archive.ReadMember("script\\foot_2.eft");
        check(data && documents.LoadBinaryDocument(root,*data) && owner.Load(root));
        documents.CleanupDocument(root);
        check(state.words[29]-state.words[28]==20u && state.words[25]-state.words[24]==24u);
        const auto baseline=memory.OutstandingAllocations();
        for(unsigned pass=0;pass<2u;++pass) {
            targets.enabled=pass==0u;
            targets.shown=targets.hidden=targets.stopped=0u;
            targets.resource[0]=0x100u;
            memory.WriteDword(state.words[28]+16u,targets.Owner());
            bool live_effect_rejected=false;
            try {owner.Clear();} catch(const std::logic_error&) {live_effect_rejected=true;}
            check(live_effect_rejected && memory.ReadDword(state.words[28]+16u)==targets.Owner());
            GomTaskContainerState container;
            const auto manager=address(&container);
            tasks.InitializeTaskContainer(manager);state.words[2]=manager;
            bool live_tasks_rejected=false;
            try {owner.Clear();} catch(const std::logic_error&) {live_tasks_rejected=true;}
            check(live_tasks_rejected && state.words[2]==manager);
            check(DialogObject::InitializeTasks(state,address(&state),task_runtime) && factory.LiveTasks()==3u && tasks.RegisteredTasks()==3u);
            std::array<std::uint32_t,3> task{};
            for(unsigned i=0;i<3u;++i) {
                task[i]=memory.ReadDword(state.words[24]+i*8u+4u);
                check(memory.ReadDword(task[i]+68u)==address(&state) &&
                    memory.ReadDword(task[i]+64u)==targets.Owner() && memory.ReadDword(task[i]+60u)==0u);
            }
            check(std::string_view(reinterpret_cast<const char*>(memory.ReadDword(task[1]+8u)))=="DecressTask" &&
                std::bit_cast<float>(memory.ReadDword(task[1]+20u))==0.7f);
            (void)tasks.BeginWaiting(manager);
            for(unsigned frame=1u;frame<=50u;++frame) {
                (void)tasks.UpdateTaskContainer(manager,0.05f);
                if(frame==10u) check(targets.shown==1u && !targets.stopped && !targets.hidden && memory.ReadByte(targets.Owner()+6u)==1u);
                if(frame==20u) check(targets.stopped==1u && !targets.hidden && factory.LiveTasks()==3u &&
                    !memory.ReadByte(address(targets.resource.data())+1u) && memory.ReadDword(task[1]+40u)==3u);
                if(frame==30u) check(!targets.hidden && memory.ReadDword(task[1]+40u)==4u);
                if(frame==40u) check(!targets.hidden);
            }
            check(targets.shown==1u && targets.stopped==1u && targets.hidden==1u &&
                !memory.ReadByte(targets.Owner()+6u) && container.ready && !container.count &&
                !factory.LiveTasks() && !tasks.RegisteredTasks());
            (void)tasks.DestroyTaskContainer(manager);state.words[2]=0u;
            (void)DialogObject::ResetTaskSlots(state,task_runtime);
            memory.WriteDword(state.words[28]+16u,0u);
            check(memory.OutstandingAllocations()==baseline);
        }
        owner.Clear();
        check(state.words[25]==state.words[24] && state.words[29]==state.words[28] &&
            state.words[33]==state.words[32] && !state.words[38] && !state.words[42]);
        state.words[43]=0x12345678u;
        bool live_frame_rejected=false;
        try {owner.Clear();} catch(const std::logic_error&) {live_frame_rejected=true;}
        check(live_frame_rejected && state.words[43]==0x12345678u);state.words[43]=0u;
        // Leave a freshly loaded configuration for automatic owner destruction.
        check(documents.LoadBinaryDocument(root,*data) && owner.Load(root));
        documents.CleanupDocument(root);
    }
    const auto owner_baseline=memory.OutstandingAllocations();
    {NativeDialogObject recreated(memory,documents);check(recreated.FacetNil() && recreated.TargetNil());}
    check(memory.OutstandingAllocations()==owner_baseline && documents.DocumentCount()==1u);
    {
        NativePackageArchive archive;
        const auto path=(game_root/"gamedata"/"Effect.cpk").string();
        check(archive.Open(path.c_str()));const auto data=archive.ReadMember("script\\foot_2.eft");check(data.has_value());
        for(unsigned branch=0;branch<3u;++branch) {
            NativeDialogObject owner(memory,documents);auto& state=owner.State();
            check(documents.LoadBinaryDocument(root,*data) && owner.Load(root));documents.CleanupDocument(root);
            NativeDialogPreDeleteRuntime cleanup(memory,tasks,documents,owner.Templates(),targets);
            targets.enabled=true;targets.active=false;
            targets.modes=targets.references=targets.events=targets.destroyed=0u;
            auto* parent=RwFrameCreate();auto* frame=RwFrameCreate();targets.effect_frame=RwFrameCreate();
            check(parent && frame && targets.effect_frame && RwFrameAddChild(parent,frame) &&
                RwFrameAddChild(frame,targets.effect_frame));
            state.words[43]=address(frame);state.words[3]=7u;
            memory.WriteDword(state.words[28]+16u,targets.Owner());
            const auto container=memory.Allocate(sizeof(GomTaskContainerState));
            tasks.InitializeTaskContainer(container);state.words[2]=container;
            check(DialogObject::InitializeTasks(state,owner.Address(),task_runtime) && factory.LiveTasks()==3u);
            (void)tasks.BeginWaiting(container);(void)tasks.UpdateTaskContainer(container,0.125f);
            targets.enabled=branch==0u;targets.active=branch==2u;
            documents.WriteStringAttribute(owner.Address()+20u,"cleanup-probe","retained");
            owner.Properties().SetFacet("T1",0x1234u);
            cleanup.Prepare(owner);
            check(!owner.Properties().FacetCount());
            check(!state.words[2] && !state.words[43] && !state.words[3] && !factory.LiveTasks() && !tasks.RegisteredTasks());
            check(state.words[25]==state.words[24] && state.words[29]==state.words[28] && state.words[33]==state.words[32]);
            check(!owner.Templates().Find("lz.1") && !documents.FindNumericAttribute(owner.Address()+20u,"cleanup-probe"));
            unsigned child_count{};
            (void)RwFrameForAllChildren(parent,[](RwFrame* child,void* p)->RwFrame* {
                ++*static_cast<unsigned*>(p);return child;
            },&child_count);
            check(targets.destroyed==1u && !targets.effect_frame && !child_count &&
                targets.modes==(branch==2u?0u:1u) && targets.references==(branch==2u?0u:2u) &&
                targets.events==(branch==2u?0u:2u) && targets.parent_at_destroy==(branch==2u));
            check(RwFrameDestroy(parent));
            // A second prepare is harmless and does not redispatch dead effects.
            cleanup.Prepare(owner);check(targets.destroyed==1u);
        }
        targets.active=false;
    }
    check(!factory.LiveTasks() && !tasks.RegisteredTasks() && documents.DocumentCount()==1u &&
        memory.OutstandingAllocations()==owner_baseline);
    const auto load=[&](const BinaryXmlNode& node) {
        const auto bytes=BinaryXmlConfig::Serialize(node);
        check(bytes && documents.LoadBinaryDocument(root,*bytes));
    };
    load({"task",{{"Attr_Class_Name",3u,std::string("DecressTask")},
        {"Attr_Task_Decress_Decress",1u,std::int32_t{1}},
        {"Attr_Task_Decress_Incress",1u,std::int32_t{1}},
        {"Attr_Task_Decress_Time",2u,0.5f}}, {}});
    targets.resource[50]=100u;targets.resource[52]=20u;
    const auto increase=factory.CreateTask(root);
    check(increase && factory.LiveTasks()==1u);
    memory.WriteDword(increase+64u,targets.Owner());
    tasks.BeginRun(increase);
    check(targets.emitted==1u && targets.resource[5]==160u && targets.resource[6]==20u &&
        memory.ReadByte(address(targets.resource.data())+16u)==1u);
    tasks.DeleteTask(increase,1u);
    // Copy-timing is independent of the Dialog pointer at+68.
    documents.WriteIntAttribute(root,"Attr_Inherit_Pre_Task",1);
    const auto inherited=factory.CreateTask(root);
    GomTaskBaseState previous;
    previous.wait_mode=2u;previous.wait_time=0.25f;previous.expect_time=0.75f;
    memory.WriteDword(inherited+48u,address(&previous));
    factory.BindDialogObject(inherited,0x12345678u);factory.PrepareTask(inherited);
    check(memory.ReadByte(inherited+60u)==0u && memory.ReadByte(inherited+61u)==1u &&
        memory.ReadDword(inherited+68u)==0x12345678u && memory.ReadDword(inherited+12u)==2u &&
        std::bit_cast<float>(memory.ReadDword(inherited+16u))==0.25f &&
        std::bit_cast<float>(memory.ReadDword(inherited+20u))==0.75f);
    tasks.DeleteTask(inherited,1u);
    // Lifecycle dispatch remains owned by the task registry after factory release.
    std::uint32_t retained{};
    {NativeEffectTaskFactory temporary(memory,documents,tasks,targets);retained=temporary.CreateTask(root);check(retained);}
    tasks.DeleteTask(retained,1u);
    load({"task",{{"Attr_Class_Name",3u,std::string("DecressTask")},
        {"Attr_Task_Decress_Decress",3u,std::string("1")}}, {}});
    const auto before=memory.OutstandingAllocations();bool conversion_failed=false;
    try {(void)factory.CreateTask(root);} catch(const std::logic_error&) {conversion_failed=true;}
    check(conversion_failed && memory.OutstandingAllocations()==before && !factory.LiveTasks() && !tasks.RegisteredTasks());
    // Result scaling, rate scaling and rollback all use the actual scheduler.
    for(unsigned variant=0;variant<3u;++variant) {
        const bool rollback=variant==2u;
        load({"task",{{"Attr_Class_Name",3u,std::string("ScaleTask")},
            {"Attr_Task_Scale_Result",1u,std::int32_t{variant!=1u}},
            {"Attr_Task_Scale_Rollback",1u,std::int32_t{rollback}},
            {"Attr_Task_Scale_x",2u,3.0f},{"Attr_Task_Scale_y",2u,5.0f},
            {"Attr_Task_Scale_z",2u,7.0f},
            {"Attr_Time_Interval",2u,0.25f},{"Attr_Time_Interval2",2u,0.125f}}, {}});
        for(unsigned i=25u;i<28u;++i) targets.resource[i]=std::bit_cast<std::uint32_t>(1.0f);
        const auto baseline=memory.OutstandingAllocations();
        GomTaskContainerState container;
        const auto manager=address(&container);tasks.InitializeTaskContainer(manager);
        const auto task=factory.CreateTask(root);
        memory.WriteDword(task+64u,targets.Owner());
        tasks.AddTask(manager,task,"ScaleTask",{3u,0.0f,0.5f});
        (void)tasks.BeginWaiting(manager);
        for(unsigned n=0;n<4u && tasks.TaskAt(task).phase!=3u;++n)
            (void)tasks.UpdateTaskContainer(manager,0.125f);
        check(tasks.TaskAt(task).phase==3u);
        const auto scale=[&](unsigned i){return std::bit_cast<float>(targets.resource[25u+i]);};
        (void)tasks.UpdateTaskContainer(manager,0.125f);
        check(scale(0)==(variant==1u?1.375f:1.5f) && scale(1)==(variant==1u?1.625f:2.0f));
        for(unsigned n=0;n<3u;++n) (void)tasks.UpdateTaskContainer(manager,0.125f);
        if(!rollback) {
            check(tasks.TaskAt(task).phase==4u && scale(0)==(variant==1u?2.5f:3.0f) &&
                scale(1)==(variant==1u?3.5f:5.0f) && !memory.ReadDword(task+100u));
            (void)tasks.UpdateTaskContainer(manager,0.125f);
            check(container.ready && !factory.LiveTasks());
        } else {
            check(tasks.TaskAt(task).phase==3u && !tasks.IsDurationComplete(task) && scale(0)==3.0f);
            for(unsigned n=0;n<2u;++n) (void)tasks.UpdateTaskContainer(manager,0.125f);
            check(scale(0)==3.0f); // Forward endpoint pause lasts 0.25s.
            for(unsigned n=0;n<4u;++n) (void)tasks.UpdateTaskContainer(manager,0.125f);
            check(scale(0)==1.0f && scale(1)==1.0f && scale(2)==1.0f && tasks.TaskAt(task).phase==3u);
            (void)tasks.UpdateTaskContainer(manager,0.125f);
            check(scale(0)==1.0f); // Return endpoint pause lasts 0.125s.
            (void)tasks.UpdateTaskContainer(manager,0.125f);
            check(scale(0)==1.5f && !tasks.IsDurationComplete(task));
        }
        (void)tasks.DestroyTaskContainer(manager);
        check(!factory.LiveTasks() && !tasks.RegisteredTasks() && memory.OutstandingAllocations()==baseline);
    }
    check(targets.frame_updates>0u);
    // TrackTask is now supported; malformed point lists still fail with
    // the original error and must retire their native task and STL owner.
    load({"task",{{"Attr_Class_Name",3u,std::string("TrackTask")}}, {}});
    const auto malformed_baseline=memory.OutstandingAllocations();bool malformed=false;
    try{(void)factory.CreateTask(root);}catch(const std::runtime_error& e){
        if(std::string_view(e.what())!="create track task failed,because the track points is less than 4...")throw;
        malformed=true;
    }
    check(malformed && memory.OutstandingAllocations()==malformed_baseline && !factory.LiveTasks());
    load({"task",{{"Attr_Class_Name",3u,std::string("SlowMotionTask")}}, {}});
    const auto unsupported_baseline=memory.OutstandingAllocations();bool unsupported=false;
    try {(void)factory.CreateTask(root);} catch(const std::logic_error&) {unsupported=true;}
    check(unsupported && memory.OutstandingAllocations()==unsupported_baseline && !factory.LiveTasks());
    load({"task",{{"Attr_Class_Name",3u,std::string("unknown-task")}}, {}});
    check(!factory.CreateTask(root) && !factory.CreateTask(0u));
    load({"task",{}, {}});check(!factory.CreateTask(root));
    documents.DestroyDocument(root);
    check(!factory.LiveTasks() && !tasks.RegisteredTasks() && !documents.DocumentCount() &&
        !documents.SharedNil() && !memory.OutstandingAllocations());
}
}  // namespace pal4::test
