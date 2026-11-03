
#include "pal4/native_gob_local_float_runtime.h"
#include "pal4/native_gob_machine_runtime.h"
#include "pal4/native_gob_clump_asset.h"
#include "pal4/native_scene_gob_catalog.h"
#include "pal4/native_scene_movement_asset.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
void VerifyNativeM08PlatformRuntime(const std::filesystem::path& root,
    IGameResourceManagerRuntime& transforms, NativeBaseObjectMemory& memory,
    IRwDictSchemaRuntime& schema) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto allocations=memory.AllocationAddresses();
    NativeSceneGobCatalog catalog;
    check(catalog.Load(root,"M08","2"),"LocalFloat runtime actual second-floor catalog");
    const auto found=std::find_if(catalog.Records().begin(),catalog.Records().end(),
        [](const auto& r){return r.name=="floor01";});
    check(found!=catalog.Records().end() && found->type==2u,"LocalFloat runtime real floor01 type");
    {
        NativeGobClumpAsset model;
        check(model.Load(root,found->resource_dir,found->model),"LocalFloat runtime actual model");
        NativeGobObjectImage object(memory,schema);
        (void)object.Construct(2u);const auto owner=object.Address();
        check(memory.ReadDword(owner)==GobLocalFloat::kVtable && memory.ReadDword(owner+0x2d8u)==2u &&
              memory.ReadDword(owner+0x2ccu)==GobLocalFloat::kHelperVtable,"shared derived constructor and helper image");
        object.BindRecord(*found,reinterpret_cast<std::uint32_t>(model.Clump()),transforms);
        check(!memory.ReadDword(owner+0x2d4u) && !memory.ReadDword(owner+0x2d8u) &&
              !memory.ReadDword(owner+0x190u) && memory.ReadDword(owner+0x198u)==0xffffffffu,
              "original floor01 touch configuration and inherited unlimited uses");
        check(!object.CaptureSaveDocument(),"original floor01 store0 omits snapshot");
        NativeGobMachineServices services;
        NativeGobMachineRuntime machine(object,*found,memory,std::move(services));
        check(machine.Trigger()!=0u && machine.TaskCount()==1u,
              "derived config builds real script2 task before script execution");
        // Do not advance or invoke the original story function in this fixture.
        (void)object.SetVisible(0u);
        check(!machine.TaskCount(),"LocalFloat visibility clears inherited task ownership");
    }
    check(memory.AllocationAddresses()==allocations,"real LocalFloat constructor/config/task cancellation cleanup");
    // Isolated numeric carry/snapshot fixture: same model/base config, replace
    // only the script task with Move0, enable storage, retain touch0. No game
    // resources or live story state are modified.
    auto record=*found;
    auto xml=record.config.Root();bool replaced{},stored{};
    const auto initial=[&](unsigned axis){return std::bit_cast<float>(record.position_bits[axis]);};
    for(auto& a:xml.attributes)if(a.name=="PAL4-GameObject-object-store"){a.value=std::int32_t(1);a.type=1u;stored=true;}
    // Base parameter attributes can be attached to the root or its parameters.
    const auto enable_store=[&](auto&& self,BinaryXmlNode& node)->void{
        for(auto& a:node.attributes)if(a.name=="PAL4-GameObject-object-store"){a.value=std::int32_t(1);stored=true;}
        for(auto& child:node.children)self(self,child);
    };
    enable_store(enable_store,xml);
    for(auto& parameters:xml.children)for(auto& machine:parameters.children)
        if(machine.name=="PAL4_GAMEOBJECT_float")for(auto& list:machine.children)
            if(list.name=="PAL4_GameObject-machine-tasks"){
                BinaryXmlNode move;move.name="isolated_move";
                move.attributes={{"PAL4_GOMTask_type",1u,std::int32_t(0)},
                    {"PAL4_GOMTask_wait_mode",1u,std::int32_t(1)},
                    {"PAL4_GOMTask_expt_time",2u,1.0f},
                    {"PAL4_GOMTask_move_target_X",2u,initial(0)+20.0f},
                    {"PAL4_GOMTask_move_target_Y",2u,initial(1)},
                    {"PAL4_GOMTask_move_target_Z",2u,initial(2)}};
                list.children={std::move(move)};replaced=true;
            }
    const auto bytes=BinaryXmlConfig::Serialize(xml);
    check(replaced && stored && bytes && record.config.Parse(*bytes),"LocalFloat isolated carry/store fixture");
    {
        NativeGobClumpAsset model,actor_model;
        check(model.Load(root,record.resource_dir,record.model) && actor_model.Load(root,record.resource_dir,record.model),
              "LocalFloat carry uses actual SDK platform/actor stand-in clumps");
        NativeGobObjectImage object(memory,schema),actor(memory,schema);
        (void)object.Construct(2u);object.BindRecord(record,reinterpret_cast<std::uint32_t>(model.Clump()),transforms);
        auto actor_record=*found;actor_record.type=0u;actor_record.name="platform_actor_fixture";
        (void)actor.Construct(0u);actor.BindRecord(actor_record,reinterpret_cast<std::uint32_t>(actor_model.Clump()),transforms);
        const auto owner=object.Address();
        actor.SetPosition({initial(0)+3.0f,initial(1)+10.0f,initial(2)});
        const auto snapshot=object.CaptureSaveDocument();
        check(snapshot && snapshot->Root().name=="soGOBLocalFloat","shared LocalFloat snapshot has actual type2 XML root");
        object.SetPosition({initial(0)+100.0f,initial(1),initial(2)});
        memory.WriteDword(owner+0x198u,7u);
        object.ApplySaveDocument(*snapshot);
        check(memory.ReadDword(owner+0xacu)==record.position_bits[0] && memory.ReadDword(owner+0x198u)==0xffffffffu,
              "LocalFloat snapshot restores exact position bits and uses");
        std::array<std::uint32_t,2> actor_control{1u,actor.Address()};
        unsigned grid{},floor{};
        NativeGobLocalFloatActorServices actors;
        actors.transforms=&transforms;
        actors.actor_control=[&]{return reinterpret_cast<std::uint32_t>(&actor_control);};
        actors.set_position=[&](std::uint32_t address,const std::array<float,3>& p){
            check(address==actor.Address(),"LocalFloat carry current actor ownership");actor.SetPosition(p);
        };
        actors.update_grid=[&](std::uint32_t p,std::uint32_t r){
            check(p==actor.Address()+0xacu && r==actor.Address()+0xa0u,"LocalFloat actual actor grid fields");++grid;
        };
        actors.update_floor=[&]{++floor;return 0u;};
        NativeGobMachineServices services;services.refresh_minimap=[]{};
        NativeGobMachineRuntime machine(object,record,memory,std::move(services));
        NativeSceneMovementAsset movement;check(movement.Load(root,"M08","2"),"LocalFloat actual scene collision owner");
        NativeGobLocalFloatRuntime platform(object,machine,memory,std::move(actors));
        memory.WriteDword(owner+0x198u,0u);
        check(platform.Touch(0u)==0u && !machine.TaskCount(),
              "explicit touch0 with exhausted uses does not build tasks");
        memory.WriteDword(owner+0x198u,0xffffffffu);
        (void)platform.Touch(17u);
        check(machine.TaskCount()==1u && !memory.ReadDword(owner+0x2d0u),
              "explicit LocalFloat touch builds inherited Move without helper latch");
        (void)object.SetVisible(1u);
        check(!machine.TaskCount(),"visibility resets explicit touch task before helper carry fixture");
        platform.AttachMovement(movement);
        check(movement.PlatformHelpers().Count()==1u && movement.IsObjectClumpRegistered(model.Clump()),
              "shared platform attach registers primary collision and secondary helper");
        platform.Enter();platform.Enter();
        check(memory.ReadDword(owner+0x2d0u)==1u && machine.TaskCount()==1u,"touch0 starts inherited Move once and latches");
        for(unsigned i=0;i<90u;++i)platform.Advance(.017f);
        const auto actor_x=std::bit_cast<float>(memory.ReadDword(actor.Address()+0xacu));
        std::cout << "LocalFloat carry numeric: actor_x=" << actor_x << " expected=" << initial(0)+23.0f
            << " platform_x=" << std::bit_cast<float>(memory.ReadDword(owner+0xacu))
            << " grid=" << grid << " floor=" << floor << " tasks=" << machine.TaskCount() << "\n";
        check(std::fabs(actor_x-(initial(0)+23.0f))<.02f && grid && floor==grid,
              "SDK platform translation carries actor twenty units and refreshes grid/floor");
        const auto updates=grid;
        platform.Leave();platform.Advance(.017f);
        check(!memory.ReadDword(owner+0x2d0u) && grid==updates,"leaving platform stops actor carry");
        platform.DetachMovement(movement);
        check(!movement.PlatformHelpers().Count() && !movement.IsObjectClumpRegistered(model.Clump()),
              "shared detach retires helper and collision borrowers before their owners");
    }
    check(memory.AllocationAddresses()==allocations,"LocalFloat snapshots/tasks/helper lifetime restores complete native allocation set");
    std::cout<<"Native M08 LocalFloat runtime: real derived config/script task, constructor, visibility cancellation, type2 snapshot, SDK carry, grid/floor, collision attach/detach and allocation cleanup pass\n";
}
}  // namespace pal4::test
