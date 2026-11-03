#pragma once
#include "pal4/native_scene_gob_catalog.h"
#include "pal4/native_gob_clump_asset.h"
#include "pal4/native_gob_machine_runtime.h"
#include "pal4/native_gob_animation_binding.h"
#include "pal4/native_gob_machine_aux_tasks.h"
#include "pal4/native_script_object_runtime.h"
#include "pal4/gob_machine_task_runtime.h"
#include "pal4/native_miles_sound_2d_runtime.h"
#include "pal4/gob_machine.h"
#include <rpworld.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeM07Machine(const std::filesystem::path& root,
    IGameResourceManagerRuntime& transforms,NativeBaseObjectMemory& memory,
    IRwDictSchemaRuntime& schema,NativeMilesSound2dRuntime& sound){
    const auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    NativeSceneGobCatalog catalog;check(catalog.Load(root,"M07","1"),"M07 real machine catalog");
    const auto record=std::find_if(catalog.Records().begin(),catalog.Records().end(),[](const auto& r){return r.name=="huaRongDoor01";});
    check(record!=catalog.Records().end() && record->type==GobMachine::TypeKey(),"M07 original door is Machine5");
    const auto allocated=memory.OutstandingAllocations();
    {
        NativeGobClumpAsset model;check(model.Load(root,record->resource_dir,record->model),"M07 door real clump");
        NativeGobObjectImage object(memory,schema);(void)object.Construct(record->type);
        object.BindRecord(*record,reinterpret_cast<std::uint32_t>(model.Clump()),transforms);
        const auto owner=object.Address();
        NativeGobAnimationBinding binding(object,model,memory);
        check(binding.Load(record->model) && !binding.Bound(),"M07 static door has no animation table");
        check(memory.ReadDword(owner+GobMachine::kStateOffset+76u)==1u &&
              memory.ReadDword(owner+GobMachine::kStateOffset+68u)==0u,
              "static Machine load retains original pause1 restart0 without a clip");
        const auto position=[&]{std::array<float,3> p;std::memcpy(p.data(),reinterpret_cast<const void*>(owner+0xacu),12u);return p;};
        const auto initial=position();
        {
            NativeGobMachineServices services;services.sound=&sound;
            NativeGobMachineRuntime machine(object,*record,memory,std::move(services));
            check(machine.Trigger()!=0u && machine.TaskCount()==2u && memory.ReadDword(owner+0x27cu)==1u,"Machine actual XML creates Start16 and Move0 and begins original container");
            object.Deactivate();object.Activate();object.Deactivate();object.Activate();
            check(machine.TaskCount()==2u && position()==initial && memory.ReadDword(owner+0x27cu)==1u,
                "scene activity traverses live task pointers without clearing tasks or advancing transforms");
            machine.Advance(.1f);check(position()==initial,"task waiting phase applies Start before move");
            machine.Advance(.1f);machine.Advance(.1f);machine.Advance(.1f);
            check(position()[1]>initial[1] && position()[1]<initial[1]+140.f,"real Machine container advances shared Move interpolation");
            for(unsigned i=0;i<310u;++i)machine.Advance(1.f/60.f);
            const auto final=position();
            check(final[0]==initial[0] && final[2]==initial[2] && final[1]==230.89720153808594f,"original M07 4.5-second task reaches configured final transform exactly");
            const auto* ltm=RwFrameGetLTM(RpClumpGetFrame(model.Clump()));
            check(ltm->pos.x==final[0] && ltm->pos.y==final[1] && ltm->pos.z==final[2],"Machine task updates actual RenderWare door root");
            check(!machine.TaskCount() && !memory.ReadDword(owner+0x27cu) && memory.ReadByte(owner+0x264u) && memory.ReadByte(owner+0x274u),"completed Machine tasks release names/lists and clear running state");
            object.Deactivate();object.Activate();
            check(!machine.TaskCount() && position()==final,"ready Machine scene activity preserves completion and transform");
            (void)machine.Trigger();machine.Advance(.1f);machine.Advance(.1f);machine.Advance(.1f);
            (void)object.SetVisible(0u);
            check(!machine.TaskCount() && !memory.ReadDword(owner+0x27cu),"hidden Machine clears active tasks through original visibility lifecycle");
        }
        check(!object.MachineService(),"Machine service detaches before scene object destruction");
    }
    {
        NativeSceneGobCatalog switches;
        check(switches.Load(root,"M07","2"),"M07 switch catalog");
        const auto key=std::find_if(switches.Records().begin(),switches.Records().end(),
            [](const auto& r){return r.name=="door02_key";});
        check(key!=switches.Records().end(),"M07 original animated switch");
        NativeGobClumpAsset model;check(model.Load(root,key->resource_dir,key->model),"M07 switch clump");
        NativeGobObjectImage object(memory,schema);(void)object.Construct(key->type);
        object.BindRecord(*key,reinterpret_cast<std::uint32_t>(model.Clump()),transforms);
        NativeGobAnimationBinding binding(object,model,memory);
        check(binding.Load(key->model) && binding.Bound(),"M07 switch has original action table and clips");
        NativeScriptObjectRuntime tasks(memory);NativeGobMachineServices services;
        NativeGobMachineAuxTasks auxiliary(object,memory,tasks,services);
        for(const auto name:{"B","missing_action_fixture"}){
            GobMachineTaskDefinition definition;definition.type=3;
            definition.parameters={{"PAL4_GOMTask_action_time",std::int32_t(1)},
                {"PAL4_GOMTask_action_name",std::string(name)}};
            const auto task=auxiliary.Construct(3);
            auxiliary.Initialize(task,object.Address()+GobMachine::kPrimaryTasksOffset,definition);
            auxiliary.Begin(task,3);
            if(std::string_view(name)=="B"){
                unsigned frames{};
                while(!memory.ReadByte(task+0x3cu) && frames++<3600u){
                    (void)binding.AdvanceFrame(1.f/60.f);auxiliary.Tick(task,3,0.f);
                }
                check(memory.ReadByte(task+0x3cu) && binding.Time()==binding.Length() &&
                    memory.ReadDword(object.Address()+GobMachine::kStateOffset+76u)==1u,
                    "real Action3 completes one B cycle and holds its final frame");
            }else{
                auxiliary.Tick(task,3,0.f);
                check(memory.ReadByte(task+0x3cu)!=0u,
                    "missing selection retains current clip and completes via original name mismatch");
            }
            auxiliary.Destruct(task,3);(void)memory.Free(task);
        }
    }
    {
        // Retain the real door/model/timing, adding a rollback to exercise the
        // mixed primary/context scheduler through two full reverse/restart cycles.
        auto rollback_record=*record;
        auto xml=record->config.Root();bool inserted{};
        for(auto& parameters:xml.children)for(auto& machine:parameters.children)
            for(auto& list:machine.children)if(list.name=="PAL4_GameObject-machine-tasks"){
                BinaryXmlNode rollback;rollback.name="PAL4_GameObject-machine-task";
                rollback.attributes={{"PAL4_GOMTask_type",1u,std::int32_t(6)},
                    {"PAL4_GOMTask_wait_mode",1u,std::int32_t(3)}};
                list.children.push_back(std::move(rollback));inserted=true;
            }
        const auto bytes=BinaryXmlConfig::Serialize(xml);
        check(inserted && bytes && rollback_record.config.Parse(*bytes),"rollback fixture retains original door configuration");
        NativeGobClumpAsset model;check(model.Load(root,record->resource_dir,record->model),"rollback real door clump");
        NativeGobObjectImage object(memory,schema);(void)object.Construct(record->type);
        object.BindRecord(rollback_record,reinterpret_cast<std::uint32_t>(model.Clump()),transforms);
        const auto owner=object.Address();
        const auto y=[&]{return std::bit_cast<float>(memory.ReadDword(owner+0xb0u));};
        const auto initial=y();unsigned tops{},bottoms{};bool at_top{},at_bottom=true;
        NativeGobMachineServices services;services.sound=&sound;
        NativeGobMachineRuntime machine(object,rollback_record,memory,std::move(services));
        check(memory.ReadDword(owner+0x2b0u)==1u && machine.Trigger()!=0u && machine.TaskCount()==3u,
            "rollback metadata and original Start/Move/Rollback registration");
        for(unsigned i=0;i<8u;++i)machine.Advance(.1f);
        check(y()>initial && y()<230.89720153808594f && machine.TaskCount()==3u &&
            memory.ReadByte(owner+0x274u)!=0u,"rollback custom wait holds while its Move sibling is incomplete");
        for(unsigned i=0;i<260u;++i){
            machine.Advance(.1f);
            const bool top=y()==230.89720153808594f,bottom=y()==initial;
            if(top && !at_top)++tops;if(bottom && !at_bottom)++bottoms;
            at_top=top;at_bottom=bottom;
            check(machine.TaskCount()<=6u,"rollback cycles retain at most the two owned three-task containers");
        }
        check(tops>=2u && bottoms>=2u,"reverse Move/Start and StoredTransform helper restart multiple real door cycles");
        (void)object.SetVisible(0u);
        check(!machine.TaskCount() && !memory.ReadDword(owner+0x27cu) &&
            memory.ReadByte(owner+0x264u) && memory.ReadByte(owner+0x274u),
            "visibility cancellation retires primary and spawned rollback tasks");
        std::cout<<"Native Machine rollback: real door transform, sibling wait, reverse task order, two restart cycles and cancellation pass\n";
    }
    sound.Update();
    check(memory.OutstandingAllocations()==allocated,"isolated M07 machine releases task/container/pointer allocations");
    std::cout<<"Native M07 Machine: actual XML Start16+Move0, wc001 sound, original task timing, exact raised door/root, visibility cancellation and owner cleanup verified\n";
}
}
