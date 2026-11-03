#pragma once
#include "native_m07_machine_probe.h"
#include "pal4/native_camera_manager_owner.h"
#include "pal4/script_object_visibility.h"
#include <bit>
#include <map>
#include <vector>

namespace pal4::test {
inline void VerifyNativeM07QuakeActivity(const std::filesystem::path& root,
    IGameResourceManagerRuntime& transforms,NativeBaseObjectMemory& memory,
    IRwDictSchemaRuntime& schema,NativeMilesSound2dRuntime& sound,NativeCameraManagerOwner& cameras){
    const auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    // This observer verifies dispatch/registration order only. Actual particle
    // creation/rendering remains a separate paired live-scene acceptance.
    class Objects final : public IScriptObjectVisibilityRuntime {
    public:
        std::vector<std::string> events;
        std::map<std::string,std::uint32_t> names;
        std::uint32_t ObjectManager() override{return 0x1000u;}
        std::uint32_t FindByName(std::uint32_t manager,std::string_view name) override{
            if(manager!=ObjectManager())throw std::runtime_error("activity lookup manager");
            events.push_back("find:"+std::string(name));const auto i=names.find(std::string(name));return i==names.end()?0u:i->second;
        }
        std::uint32_t ArenaManager() override{return 0x2000u;}
        void RemoveAnimatedObject(std::uint32_t arena,std::uint32_t object) override{
            if(arena!=ArenaManager())throw std::runtime_error("activity remove arena");events.push_back("remove:"+std::to_string(object));
        }
        void SetObjectState(std::uint32_t object,std::uint32_t state) override{events.push_back("state:"+std::to_string(object)+":"+std::to_string(state));}
        void AddAnimatedObject(std::uint32_t arena,std::uint32_t object) override{
            if(arena!=ArenaManager())throw std::runtime_error("activity add arena");events.push_back("add:"+std::to_string(object));
        }
        std::uint32_t MinimapManager() override{throw std::logic_error("activity does not explicitly refresh minimap");}
        void RefreshDialog(std::uint32_t) override{throw std::logic_error("activity does not explicitly refresh minimap");}
        std::uint32_t ReadWord(std::uint32_t) override{throw std::logic_error("activity has no foreign reads");}
        std::uint32_t LogVisibility(std::uint32_t,std::string_view,std::uint32_t,std::uint32_t,std::uint8_t) override{throw std::logic_error("activity is not a script visibility log");}
    } objects;
    NativeSceneGobCatalog catalog;check(catalog.Load(root,"M07","4"),"M07/4 actual quake/activity catalog");
    const auto allocated=memory.OutstandingAllocations();
    const auto camera=cameras.ActiveCameraInternalId();
    auto& data=cameras.CameraDataAt(camera);const auto saved=data;
    auto* frame=RwCameraGetFrame(reinterpret_cast<RwCamera*>(data.At(0u)));
    const auto matrix=*RwFrameGetMatrix(frame);
    struct Restore{CameraDataState& data;CameraDataState saved;RwFrame* frame;RwMatrix matrix;
        ~Restore(){data=saved;RwMatrixCopy(RwFrameGetMatrix(frame),&matrix);(void)RwFrameUpdateObjects(frame);}
    } restore{data,saved,frame,matrix};
    {
        const auto chest=std::find_if(catalog.Records().begin(),catalog.Records().end(),
            [](const auto& r){return r.name=="item101";});
        check(chest!=catalog.Records().end(),"M07/4 unopened chest resource exists");
        NativeGobClumpAsset model;
        check(model.Load(root,chest->resource_dir,chest->model),"M07/4 original chest clump");
        NativeGobObjectImage object(memory,schema);(void)object.Construct(chest->type);
        object.BindRecord(*chest,reinterpret_cast<std::uint32_t>(model.Clump()),transforms);
        const auto owner=object.Address();
        const auto metadata=[&]{
            const auto name=memory.ReadDword(owner+0x2c0u);
            return memory.ReadDword(owner+0x2b8u)==1u && name &&
                std::strcmp(reinterpret_cast<const char*>(name),"OM08")==0;
        };
        check(metadata() && !object.MachineService() && !memory.ReadDword(owner+0x27cu),
            "config load records OM08 before any Machine trigger or service binding");
        NativeGobAnimationBinding binding(object,model,memory);
        check(binding.Load(chest->model) && binding.Bound(),"M07/4 original chest animation binding");
        const auto actual_clip=[&]{
            const auto record=memory.ReadDword(owner+GobMachine::kStateOffset+12u);
            const auto name=record?memory.ReadDword(record+20u):0u;
            return name && std::strcmp(reinterpret_cast<const char*>(name),"OM10")==0;
        };
        check(metadata() && actual_clip(),"saved request OM08 remains distinct from actual initial OM10 clip");
        const auto snapshot=object.CaptureSaveDocument();
        check(snapshot.has_value(),"unopened chest captures original save fields");
        object.ApplySaveDocument(*snapshot);
        check(metadata() && actual_clip(),"unopened chest snapshot roundtrip retains metadata and actual clip");
        object.ReloadConfiguration(*chest);
        check(metadata() && actual_clip() && !memory.ReadDword(owner+0x27cu),
            "Machine config reload retains metadata without starting tasks");
    }
    check(memory.OutstandingAllocations()==allocated,"unopened chest metadata cleanup has no allocations left");
    unsigned index{};
    for(const auto name:{"MO004","MO005"}){
        data=saved;objects.names.clear();objects.events.clear();
        const std::string first=index?"dust03":"dust01",second=index?"dust04":"dust02";
        objects.names.emplace(first,1u);objects.names.emplace(second,2u);
        const auto it=std::find_if(catalog.Records().begin(),catalog.Records().end(),[&](const auto& r){return r.name==name;});
        check(it!=catalog.Records().end(),"real M07 mechanism exists");
        {
            NativeGobClumpAsset model;check(model.Load(root,it->resource_dir,it->model),"real M07 mechanism clump");
            NativeGobObjectImage object(memory,schema);(void)object.Construct(it->type);
            object.BindRecord(*it,reinterpret_cast<std::uint32_t>(model.Clump()),transforms);
            NativeGobAnimationBinding binding(object,model,memory);check(binding.Load(it->model),"mechanism animation bind");
            NativeGobMachineServices services;services.sound=&sound;services.cameras=&cameras;services.objects=&objects;
            unsigned refreshes{},readds{};
            const bool expected_readd=memory.ReadDword(object.Address()+0x190u)!=0u;
            services.refresh_minimap=[&]{++refreshes;};
            services.queue_fade_out=[&](std::uint32_t address){check(address==object.Address(),"mechanism readd retains owner");++readds;};
            NativeGobMachineRuntime machine(object,*it,memory,std::move(services));
            check(machine.Trigger()!=0u && machine.TaskCount()==5u,"original chain includes Start/Move/Quake/Active/Active");
            bool saw_quake{},saw_motion{};
            for(unsigned i=0;i<100u;++i){
                machine.Advance(.1f);
                if(data.At(0x8cu)){
                    saw_quake=true;
                    check(data.At(0x94u)==std::bit_cast<std::uint32_t>(1.5f) &&
                        data.At(0x98u)==std::bit_cast<std::uint32_t>(20.f) &&
                        data.At(0x9cu)==std::bit_cast<std::uint32_t>(55.f),"quake maps original time/amplitude/frequency to actual camera");
                }
                (void)cameras.AdvanceActiveCamera(.1f);
                if(data.At(0x8cu) && data.At(0x78u)!=data.At(0xa4u))saw_motion=true;
            }
            check(saw_quake && saw_motion && !data.At(0x8cu),"actual camera frame shakes then naturally terminates");
            check(!machine.TaskCount() && !memory.ReadDword(object.Address()+0x27cu),"quake uses base timing, not the low byte of float duration");
            check(refreshes==1u && readds==static_cast<unsigned>(expected_readd),"spent original Machine refreshes minimap once and follows configured scene readd");
            const float y=index?244.16720581054688f:244.1573028564453f;
            check(memory.ReadDword(object.Address()+0xb0u)==std::bit_cast<std::uint32_t>(y) &&
                RwFrameGetLTM(RpClumpGetFrame(model.Clump()))->pos.y==y,"original Move target reaches actual mechanism/root");
            const std::vector<std::string> expected{"find:"+first,"remove:1","state:1:1","add:1",
                "find:"+second,"remove:2","state:2:1","add:2"};
            check(objects.events==expected,"original dust names and ordered remove/control/add are dispatched once");
            NativeScriptObjectRuntime tasks(memory);NativeGobMachineServices local;local.objects=&objects;
            NativeGobMachineAuxTasks auxiliary(object,memory,tasks,local);
            for(const auto target:{first,std::string("missing_activity_fixture")}){
                objects.events.clear();GobMachineTaskDefinition definition;definition.type=15;
                definition.parameters={{"PAL4_GOMTask_active_object_ctrl",std::int32_t(0)},
                    {"PAL4_GOMTask_active_object_name",target}};
                const auto task=auxiliary.Construct(15);auxiliary.Initialize(task,0u,definition);auxiliary.Begin(task,15);
                check(memory.ReadByte(task+0x3cu)!=0u,"activity control0 and missing target both complete");
                check(objects.events==(target==first?std::vector<std::string>{"find:"+first,"remove:1"}:
                    std::vector<std::string>{"find:"+target}),"control0 removes without control/add; missing target is a no-op");
                auxiliary.Destruct(task,15);(void)memory.Free(task);
            }
        }
        ++index;
    }
    sound.Update();check(memory.OutstandingAllocations()==allocated,"M07 quake/activity task ownership cleans up");
    std::cout<<"Native M07 quake/activity: both actual five-task resources, real door/root and camera shake/expiry, object dispatch order/control0/missing target and cleanup verified; particle registration observed only\n";
}
}
