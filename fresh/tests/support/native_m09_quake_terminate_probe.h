#pragma once
#include "native_m07_machine_probe.h"
#include "pal4/native_camera_manager_owner.h"
#include <bit>

namespace pal4::test {
inline void VerifyNativeM09QuakeTerminate(const std::filesystem::path& root,
    IGameResourceManagerRuntime& transforms,NativeBaseObjectMemory& memory,
    IRwDictSchemaRuntime& schema,NativeCameraManagerOwner& cameras){
    const auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    NativeSceneGobCatalog catalog;check(catalog.Load(root,"M09","1"),"M09/1 quake catalog");
    const auto it=std::find_if(catalog.Records().begin(),catalog.Records().end(),
        [](const auto& r){return r.name=="Jeffect002";});
    check(it!=catalog.Records().end(),"M09 Jeffect002 exists");
    const auto allocated=memory.OutstandingAllocations();
    auto& data=cameras.CameraDataAt(cameras.ActiveCameraInternalId());const auto saved=data;
    struct Restore{CameraDataState& data;CameraDataState saved;~Restore(){data=saved;}} restore{data,saved};
    for(const bool cancel:{false,true}){
        data=saved;
        {
            NativeGobClumpAsset model;check(model.Load(root,it->resource_dir,it->model),"M09 original Machine model");
            NativeGobObjectImage object(memory,schema);(void)object.Construct(it->type);
            object.BindRecord(*it,reinterpret_cast<std::uint32_t>(model.Clump()),transforms);
            NativeGobAnimationBinding binding(object,model,memory);check(binding.Load(it->model),"M09 original animation");
            NativeGobMachineServices services;services.cameras=&cameras;
            services.refresh_minimap=[]{};services.queue_fade_out=[](std::uint32_t){};
            NativeGobMachineRuntime machine(object,*it,memory,std::move(services));
            check(machine.Trigger()!=0u && machine.TaskCount()==2u,"real M09 quake/terminate chain constructs");
            machine.Advance(.5f);
            check(data.At(0x8cu)!=0u && data.At(0x94u)==std::bit_cast<std::uint32_t>(10.f) &&
                data.At(0x98u)==std::bit_cast<std::uint32_t>(5.f) &&
                data.At(0x9cu)==std::bit_cast<std::uint32_t>(1.f),"M09 original quake parameters reach active camera");
            if(cancel){
                (void)machine.ClearContainer(object.Address()+GobMachine::kPrimaryTasksOffset);
            }else{
                machine.Advance(.5f);
                check(data.At(0x8cu)!=0u,"quake remains active before terminate wait threshold");
                machine.Advance(.5f);
                check(data.At(0x8cu)==0u,"type21 terminates at 1.5 seconds before natural 10-second expiry");
                machine.Advance(.5f); // Running -> finished.
                machine.Advance(.5f); // Container retires finished tasks on its next pass.
                check(!machine.TaskCount(),"type21 base duration completes and tasks are released");
            }
        }
        check(memory.OutstandingAllocations()==allocated,"M09 quake/terminate normal and cancellation ownership cleanup");
    }
    std::cout<<"Native M09 quake terminate: real Jeffect002 starts ten-second quake, stops at scheduled 1.5s; normal/cancel cleanup verified\n";
}
}

