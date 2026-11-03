#pragma once
#include "pal4/native_gob_machine_runtime.h"
#include "pal4/native_gob_clump_asset.h"
#include "pal4/native_gob_animation_binding.h"
#include "pal4/native_scene_gob_catalog.h"
#include "pal4/native_gob_random_delay.h"
#include "pal4/script_task.h"
#include "pal4/time_wait_task.h"
#include <algorithm>
#include <bit>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeM10TimeWait(const std::filesystem::path& root,
    IGameResourceManagerRuntime& transforms,NativeBaseObjectMemory& memory,IRwDictSchemaRuntime& schema){
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    NativeSceneGobCatalog catalog;check(catalog.Load(root,"M10","1"),"TimeWait original M10 catalog");
    const auto found=std::find_if(catalog.Records().begin(),catalog.Records().end(),[](const auto& r){return r.name=="calCtrl";});
    check(found!=catalog.Records().end() && found->type==5u && found->model=="JDumy","TimeWait actual calCtrl/JDumy");
    const auto allocations=memory.AllocationAddresses();
    // The public helper must still reseed and draw for equal endpoints.
    NativeCrtSeedRandom(1234u,__FILE__);(void)NativeCrtRandom(__FILE__);const auto second=NativeCrtRandom(__FILE__);
    check(NativeGobRandomDelay(90.f,90.f,1234.f)==90.f && NativeCrtRandom(__FILE__)==second,"TimeWait equal bounds preserve public CRT draw contract");
    for(const bool rollback:{false,true}){
        auto record=*found;
        if(rollback){
            auto config=record.config.Root();bool appended=false;
            for(auto& parameters:config.children)if(parameters.name=="PAL4-GameObject-Parameters")
                for(auto& machine:parameters.children)if(machine.name=="PAL4_GameObject-machine")
                    for(auto& tasks:machine.children)if(tasks.name=="PAL4_GameObject-machine-tasks"){
                        BinaryXmlNode task;task.name="native-timewait-rollback-fixture";
                        task.attributes={{"PAL4_GOMTask_type",1u,std::int32_t{6}},{"PAL4_GOMTask_wait_mode",1u,std::int32_t{1}},
                            {"PAL4_GOMTask_wait_time",2u,0.f},{"PAL4_GOMTask_expt_time",2u,0.f}};
                        tasks.children.push_back(std::move(task));appended=true;
                    }
            const auto bytes=BinaryXmlConfig::Serialize(config);check(appended && bytes && record.config.Parse(*bytes),"TimeWait explicit rollback fixture serialization");
        }
        {
            NativeGobClumpAsset model;check(model.Load(root,record.resource_dir,record.model),"TimeWait actual JDumy clump");
            NativeGobObjectImage object(memory,schema);const auto owner=object.Construct(5u);
            object.BindRecord(record,reinterpret_cast<std::uint32_t>(model.Clump()),transforms);
            NativeGobAnimationBinding animation(object,model,memory);check(animation.Load(record.model),"TimeWait inherited original animation");
            unsigned pushes{},loads{},initializes{},cleanups{};NativeGobMachineServices services;
            const auto key=std::string(ScriptTask::kScriptPath)+"[func7005]";
            services.script.actor_active=[]{return true;};services.script.busy=[]{return false;};
            services.script.initialize=[&]{++initializes;};services.script.cleanup=[&]{++cleanups;};
            services.script.has_module=[&](auto path){check(path==key,"TimeWait actual script cache key");return false;};
            services.script.load_module=[&](auto path){check(path==key,"TimeWait actual script module");++loads;};
            services.script.push_module=[&](auto path,auto entry){check(path==key && entry=="func7005","TimeWait original func7005 entry");++pushes;};
            services.script.has_path=[](auto){return false;};services.script.arena_context=owner;
            services.refresh_minimap=[]{};services.queue_fade_out=[&](auto p){check(p==owner,"TimeWait research owner");};
            NativeGobMachineRuntime machine(object,record,memory,std::move(services));
            (void)object.SetVisible(1u);
            check(memory.ReadDword(owner+0x280u)==1u && machine.TaskCount()==(rollback?3u:2u),"TimeWait original repeat and task count");
            auto begin=memory.ReadDword(owner+0x2a4u);const auto task=memory.ReadDword(begin);
            check(memory.ReadDword(task)==TimeWaitTask::kVtable && memory.ReadDword(task+0x48u)==owner &&
                memory.ReadDword(task+0x4cu)==owner+0x268u && memory.ReadDword(task+0x40u)==std::bit_cast<unsigned>(90.f) &&
                memory.ReadDword(task+0x44u)==0u,"TimeWait real runtime vtable/owner/context/90-second duration");
            const auto elapsed=[&]{return std::bit_cast<float>(memory.ReadDword(task+0x44u));};
            for(unsigned i=0;i<190u && elapsed()<90.f;++i)machine.Advance(.5f);
            check(elapsed()==90.f && !memory.ReadByte(task+0x3cu) && !pushes,"TimeWait equality does not finish or run following script");
            machine.Advance(.5f);check(memory.ReadByte(task+0x3cu)!=0u,"TimeWait strict duration completion");
            // Mode1 waits strictly beyond the predecessor's finish timestamp;
            // repeated zero-delta frames cannot release that wait0 boundary.
            for(unsigned i=0;i<8u && !pushes;++i)machine.Advance(.017f);
            if(!(pushes==1u && loads==1u && initializes==1u && cleanups==1u))
                throw std::runtime_error("TimeWait actual script2 follows only after expiry: rollback="+std::to_string(rollback)+
                    " pushes="+std::to_string(pushes)+" loads="+std::to_string(loads)+" initializes="+std::to_string(initializes)+
                    " cleanups="+std::to_string(cleanups)+" tasks="+std::to_string(machine.TaskCount()));
            if(rollback){
                for(unsigned i=0;i<12u && machine.TaskCount()<5u;++i)machine.Advance(.017f);
                begin=memory.ReadDword(owner+0x2a4u);const auto end=memory.ReadDword(owner+0x2a8u);
                check(end-begin==8u && machine.TaskCount()==5u,"TimeWait native rollback creates reverse and helper in owned containers");
                const auto reverse=memory.ReadDword(begin);
                check(reverse!=task && memory.ReadDword(reverse)==TimeWaitTask::kVtable && memory.ReadDword(reverse+0x48u)==owner &&
                    memory.ReadDword(reverse+0x4cu)==0u && memory.ReadDword(reverse+0x40u)==std::bit_cast<unsigned>(90.f),
                    "TimeWait followup copies duration and clears reverse context without another random draw");
            }
            (void)machine.ResetTriggeredRuntime();check(machine.TaskCount()==0u,"TimeWait normal reset retires primary/reverse/helper owners");
        }
        check(memory.AllocationAddresses()==allocations,"TimeWait calCtrl teardown balances creating memory");
    }
    std::cout<<"Native M10 TimeWait: actual calCtrl/JDumy90-second strict boundary, public equal-range draw, script2 func7005 observer, native rollback/reverse/helper and cleanup pass\n";
}
}
