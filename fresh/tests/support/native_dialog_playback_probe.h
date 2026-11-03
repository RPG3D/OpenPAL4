#pragma once
#include "pal4/native_dialog_playback_runtime.h"
#include "pal4/native_dialog_frame_runtime.h"
#include "pal4/native_dialog_reset_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_script_object_runtime.h"
#include "pal4/native_dialog_task_runtime.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_package_archive.h"
#include "pal4/binary_xml_config.h"
#include "pal4/native_effect_object_queues.h"
#include <rwcore.h>
#include <bit>
#include <memory>
#include <source_location>
#include <stdexcept>
#include <string>

namespace pal4::test {
inline void VerifyNativeDialogPlayback(const std::filesystem::path& game_root) {
    const auto check=[](bool v,const std::source_location at=std::source_location::current()) {
        if(!v) throw std::runtime_error("native Dialog playback line "+std::to_string(at.line()));
    };
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    NativeBaseObjectMemory memory;
    NativeScriptObjectRuntime tasks(memory);
    NativeEventDocumentRuntime documents(memory);
    struct Content final : IDialogObjectActivationRuntime, INativeDialogTaskPayloads,
        IDialogObjectEffectCleanupRuntime, INativeDialogResetEffects,
        INativeDialogRestartEffects, INativeDialogPauseEffects,
        INativeEffectObjectClassification {
        NativeBaseObjectMemory& memory;
        NativeScriptObjectRuntime& tasks;
        std::array<std::uint32_t,5> effect{};
        std::array<std::uint32_t,53> group{};
        std::unique_ptr<NativeEffectObjectQueues> queues;
        RwFrame* frame{};
        bool missing{},restart_ok=true;
        unsigned created{},deleted{},begun{},ticked{},finished{},restarted{},destroyed{};
        Content(NativeBaseObjectMemory& m,NativeScriptObjectRuntime& t):memory(m),tasks(t){
            group[1]=1u;
            queues=std::make_unique<NativeEffectObjectQueues>(memory,
                reinterpret_cast<std::uint32_t>(group.data()),*this);
        }
        std::string_view ObjectTypeName(std::uint32_t) override {return "RtkPrtStdAtomicObject";}
        bool UseFallbackQueue(std::uint32_t) override {return false;}
        bool UseCameraQueue(std::uint32_t) override {return false;}
        std::uint32_t ReadWord(std::uint32_t a,std::uint32_t o) override {return memory.ReadDword(a+o);}
        void WriteWord(std::uint32_t a,std::uint32_t o,std::uint32_t v) override {memory.WriteDword(a+o,v);}
        std::string_view ReadText(std::uint32_t a) override {return reinterpret_cast<const char*>(a);}
        std::uint32_t ResolveTextProperty(std::uint32_t,std::string_view) override {return 1u;}
        std::uint32_t HandleEffectEvent(std::uint32_t,std::uint32_t,std::uint32_t) override {
            if(missing) return 0u;
            frame=RwFrameCreate();
            if(!frame) throw std::runtime_error("fixture effect frame");
            return reinterpret_cast<std::uint32_t>(effect.data());
        }
        bool EffectsEnabled() override {return true;}
        bool EffectIsActive(std::uint32_t) override {return false;}
        // Cleanup/reset remove; the activation endpoint below queues instead.
        void ProcessEffectEvent(std::uint32_t parent,std::uint32_t object) override {
            queues->RemoveEffect(parent,object);
        }
        std::uint32_t EffectFrame(std::uint32_t) override {return reinterpret_cast<std::uint32_t>(frame);}
        void AddFrameChild(std::uint32_t p,std::uint32_t c) override {
            if(!RwFrameAddChild(reinterpret_cast<RwFrame*>(p),reinterpret_cast<RwFrame*>(c)))
                throw std::runtime_error("fixture attach");
        }
        std::uint32_t FrameParent(std::uint32_t f) override {return reinterpret_cast<std::uint32_t>(RwFrameGetParent(reinterpret_cast<RwFrame*>(f)));}
        void RemoveFrameChild(std::uint32_t f) override {(void)RwFrameRemoveChild(reinterpret_cast<RwFrame*>(f));}
        void DestroyEffect(std::uint32_t) override {
            if(frame) { (void)RwFrameDestroy(frame); frame=nullptr; }
            ++destroyed;
        }
        void SetEffectMode(std::uint32_t,std::uint32_t mode) override {
            if(mode) throw std::runtime_error("fixture mode must be zero");
        }
        bool RestartEffect(std::uint32_t) override {++restarted; return restart_ok;}
        void PauseEffect(std::uint32_t) override {}
        void ReleaseEffectReferences(std::uint32_t) override {}
        struct Task final : INativeScriptTaskLifecycle {
            Content& c;
            explicit Task(Content& c):c(c){}
            void Delete(std::uint32_t p,std::uint32_t flags) override {
                if(flags!=1u) throw std::runtime_error("fixture task deletion");
                GomTaskBase base(c.tasks); (void)base.Destroy(*reinterpret_cast<GomTaskBaseState*>(p));
                (void)c.memory.Free(p); ++c.deleted;
            }
            void Begin(std::uint32_t) override {++c.begun;}
            void Tick(std::uint32_t,float) override {++c.ticked;}
            bool IsDurationComplete(std::uint32_t) override {return true;}
            void Finish(std::uint32_t) override {++c.finished;}
        };
        std::uint32_t CreateTask(std::uint32_t) override {
            const auto p=memory.Allocate(72u);
            GomTaskBase base(tasks); (void)base.Construct(*reinterpret_cast<GomTaskBaseState*>(p));
            tasks.RegisterTask(p,std::make_shared<Task>(*this)); ++created; return p;
        }
        void BindDialogObject(std::uint32_t t,std::uint32_t o) override {memory.WriteDword(t+68u,o);}
        void PrepareTask(std::uint32_t) override {}
        void StartTask(std::uint32_t) override {}
    } content(memory,tasks);
    struct Activation final : IDialogObjectActivationRuntime {
        Content& content;
        explicit Activation(Content& c):content(c){}
        std::uint32_t ReadWord(std::uint32_t a,std::uint32_t o) override {return content.ReadWord(a,o);}
        void WriteWord(std::uint32_t a,std::uint32_t o,std::uint32_t v) override {content.WriteWord(a,o,v);}
        std::string_view ReadText(std::uint32_t a) override {return content.ReadText(a);}
        std::uint32_t ResolveTextProperty(std::uint32_t m,std::string_view n) override {return content.ResolveTextProperty(m,n);}
        std::uint32_t HandleEffectEvent(std::uint32_t d,std::uint32_t o,std::uint32_t p) override {return content.HandleEffectEvent(d,o,p);}
        bool EffectsEnabled() override {return content.EffectsEnabled();}
        bool EffectIsActive(std::uint32_t e) override {return content.EffectIsActive(e);}
        void ProcessEffectEvent(std::uint32_t p,std::uint32_t e) override {content.queues->QueueEffect(p,e);}
        std::uint32_t EffectFrame(std::uint32_t e) override {return content.EffectFrame(e);}
        void AddFrameChild(std::uint32_t p,std::uint32_t c) override {content.AddFrameChild(p,c);}
    } activation(content);
    NativeDialogTaskRuntime task_runtime(memory,documents,tasks,content);
    NativeDialogResetRuntime reset(memory,tasks,content);
    NativeDialogFrameRuntime frame_runtime(memory,tasks,reset,content);
    NativeDialogPlaybackRuntime playback(memory,tasks,reset,activation,task_runtime,content,content);
    std::array<std::uint32_t,15> document_storage{};
    const auto document=address(document_storage.data());
    (void)documents.InitializeDocument(document,"task",0u);
    // Real asset task attributes, including a nonzero fractional duration.
    NativePackageArchive archive;
    const auto path=(game_root/"gamedata"/"Effect.cpk").string();
    check(archive.Open(path.c_str()));
    const auto original=archive.ReadMember("script\\foot_2.eft");
    check(original && documents.LoadBinaryDocument(document,*original));
    const auto root_children=memory.ReadDword(document+40u);
    const auto parameters=memory.ReadDword(root_children+8u);
    const auto parameter_children=memory.ReadDword(parameters+40u);
    const auto decress=memory.ReadDword(parameter_children+4u);
    check(task_runtime.StringAttribute(decress,"Attr_Class_Name")=="DecressTask" &&
          task_runtime.IntAttribute(decress,"Attr_Task_Wait_Mode",false)==2 &&
          task_runtime.IntAttribute(decress,"Attr_Task_Wait_Mode",true)==2 &&
          task_runtime.FloatAttribute(decress,"Attr_Task_Wait_Time")==0.7f &&
          task_runtime.FloatAttribute(decress,"Attr_Task_Expect_Time")==0.7f &&
          !task_runtime.StringAttribute(decress,"missing") &&
          !task_runtime.IntAttribute(decress,"missing",false) &&
          !task_runtime.FloatAttribute(decress,"missing"));
    const BinaryXmlNode config{"native-task",{
        {"Attr_Class_Name",3u,std::string("native-xml-task")},
        {"Attr_Task_Wait_Mode",1u,std::int32_t{3}},
        {"Attr_Task_Wait_Time",2u,0.125f},
        {"Attr_Task_Expect_Time",2u,0.375f}}, {}};
    const auto encoded=BinaryXmlConfig::Serialize(config);
    check(encoded && documents.LoadBinaryDocument(document,*encoded));
    DialogObjectState state;
    state.words[3]=address(content.group.data());
    state.words[21]=std::bit_cast<std::uint32_t>(1.0f);
    state.words[47]=state.words[48]=state.words[49]=std::bit_cast<std::uint32_t>(1.0f);
    std::array<std::uint32_t,2> slot{};
    std::array<std::uint32_t,5> record{};
    std::uint32_t xml=documents.CloneDocument(document);
    // Playback must use a retained parameter copy after the loader releases XML.
    documents.DestroyDocument(document);
    const auto playback_baseline=memory.OutstandingAllocations();
    check(playback_baseline>0u);
    state.words[24]=address(slot.data()); state.words[25]=address(slot.data()+slot.size());
    state.words[28]=address(record.data()); state.words[29]=address(record.data()+record.size());
    state.words[32]=address(&xml);
    content.missing=true;
    check(!DialogObject::StartPlayback(state,playback) && !state.words[2] && !state.words[43] &&
          memory.OutstandingAllocations()==playback_baseline);
    content.missing=false;
    const auto run=[&] {
        check(DialogObject::StartPlayback(state,playback) && state.words[2] && state.words[43] && slot[1]);
        check(content.queues->Count(176u)==1u &&
              content.queues->Contains(176u,address(content.effect.data())));
        check(memory.ReadDword(slot[1]+68u)==address(&state) &&
              memory.ReadDword(slot[1]+64u)==address(content.effect.data()));
        const auto& task=*reinterpret_cast<const GomTaskBaseState*>(slot[1]);
        check(std::string_view(reinterpret_cast<const char*>(task.name))=="native-xml-task" &&
              task.wait_mode==3u && task.wait_time==0.125f && task.expect_time==0.375f);
        check(RwFrameGetParent(content.frame)==reinterpret_cast<RwFrame*>(state.words[43]));
        check(DialogObject::PausePlayback(state,frame_runtime));
        for(unsigned i=0;i<3u;++i) (void)DialogObject::UpdatePlayback(state,0.25f,frame_runtime);
        check(!state.words[2] && !slot[1] && !tasks.RegisteredTasks() &&
              !content.queues->Count(176u) && memory.OutstandingAllocations()==playback_baseline);
    };
    run();
    const auto retained_frame=state.words[43];
    run();
    check(state.words[43]==retained_frame && content.created==2u && content.deleted==2u &&
          content.begun==2u && content.ticked==2u && content.finished==2u && content.restarted==1u);
    content.restart_ok=false;
    // Resume failure retains frames when enabled; Start falls through to
    // Prepare, whose resumed-byte guard rejects rebuilding this cached object.
    check(!DialogObject::StartPlayback(state,playback) && !state.words[2] &&
          state.words[43]==retained_frame && content.restarted==2u && memory.OutstandingAllocations()==playback_baseline);
    playback.StopEffects(state);
    playback.DestroyFrame(state.words[43]); state.words[43]=0u;
    check(!record[4] && !content.frame && content.destroyed==1u && !tasks.RegisteredTasks());
    documents.DestroyDocument(xml);
    (void)documents.Free(xml);
    check(!content.queues->Count(176u));content.queues.reset();
    check(!documents.DocumentCount() && !documents.SharedNil() && !memory.OutstandingAllocations());
}
}  // namespace pal4::test
