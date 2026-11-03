#pragma once
#include "pal4/native_scene_gob_catalog.h"
#include "pal4/native_gob_clump_asset.h"
#include "pal4/native_gob_machine_runtime.h"
#include "pal4/native_miles_sound_2d_runtime.h"
#include "pal4/gob_machine.h"
#include <rpworld.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeM08Rotate(const std::filesystem::path& root,
    IGameResourceManagerRuntime& transforms,NativeBaseObjectMemory& memory,
    IRwDictSchemaRuntime& schema,NativeMilesSound2dRuntime& sound) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    NativeSceneGobCatalog catalog;check(catalog.Load(root,"M08","1"),"M08 original GOB catalog");
    const auto allocations=memory.AllocationAddresses();
    for(const auto name:{"door01","door02"}){
        const auto found=std::find_if(catalog.Records().begin(),catalog.Records().end(),
            [&](const auto& r){return r.name==name;});
        check(found!=catalog.Records().end() && found->type==GobMachine::TypeKey(),"original M08 door Machine");
        for(const bool rollback:{false,true}){
            // Real door configuration, optionally append only a rollback to
            // exercise type1's spawned Rotate/Start/helper ownership chain.
            auto record=*found;
            if(rollback){
                auto xml=record.config.Root();bool inserted{};
                for(auto& parameters:xml.children)for(auto& machine:parameters.children)
                    for(auto& list:machine.children)if(list.name=="PAL4_GameObject-machine-tasks"){
                        BinaryXmlNode task;task.name="rotate_rollback_fixture";
                        task.attributes={{"PAL4_GOMTask_type",1u,std::int32_t(6)},
                            {"PAL4_GOMTask_wait_mode",1u,std::int32_t(3)}};
                        list.children.push_back(std::move(task));inserted=true;
                    }
                const auto bytes=BinaryXmlConfig::Serialize(xml);
                check(inserted && bytes && record.config.Parse(*bytes),"M08 reverse fixture retains original rotation/sound/timing");
            }
            auto sound_before=sound.SampleHandles();std::sort(sound_before.begin(),sound_before.end());
            {
                NativeGobClumpAsset model;check(model.Load(root,record.resource_dir,record.model),"M08 actual door clump");
                NativeGobObjectImage object(memory,schema);(void)object.Construct(record.type);
                object.BindRecord(record,reinterpret_cast<std::uint32_t>(model.Clump()),transforms);
                const auto owner=object.Address();
                const auto yaw=[&]{return std::bit_cast<float>(memory.ReadDword(owner+0xa4u));};
                const auto initial=yaw();
                const auto target=std::string_view(name)=="door01"?280.0785217285156f:277.9696350097656f;
                NativeGobMachineServices services;services.sound=&sound;
                unsigned refreshes{},readds{};
                services.refresh_minimap=[&]{++refreshes;};
                services.queue_fade_out=[&](std::uint32_t p){check(p==owner,"completed Rotate registers its own scene object");++readds;};
                NativeGobMachineRuntime machine(object,record,memory,std::move(services));
                check(machine.Trigger()!=0u && machine.TaskCount()==(rollback?3u:2u),"actual XML builds Start16/Rotate1/optional Rollback6");
                machine.Advance(.1f);machine.Advance(.1f);machine.Advance(.1f);machine.Advance(.1f);
                check(yaw()!=initial && yaw()!=target,"actual SDK door advances through an intermediate rotation");
                if(std::string_view(name)=="door02")check(sound.ActiveCount()>sound_before.size(),"original Rotate WC001 creates an owned Miles handle");
                // Scene activity must dispatch the Rotate audio slots without
                // dropping task ownership or changing the transform.
                const auto paused_yaw=yaw();object.Deactivate();object.Activate();
                check(yaw()==paused_yaw && machine.TaskCount()==(rollback?3u:2u),"Rotate leave/enter keeps task and transform");
                if(!rollback){
                    for(unsigned i=0;i<240u;++i)machine.Advance(.017f);
                    check(yaw()==target,"M08 real three-second Rotate reaches exact configured yaw");
                    check(!machine.TaskCount() && !memory.ReadDword(owner+0x27cu) &&
                        memory.ReadByte(owner+0x264u) && memory.ReadByte(owner+0x274u),"finished Rotate retires both containers and running state");
                    check(refreshes==1u && readds==(memory.ReadDword(owner+0x190u)?1u:0u),
                        "one-use door completion refreshes minimap and obeys original scene registration flag");
                    const auto* ltm=RwFrameGetLTM(RpClumpGetFrame(model.Clump()));
                    check(std::isfinite(ltm->at.x) && std::isfinite(ltm->at.z),"rotated SDK root remains finite");
                }else{
                    unsigned opened{},closed{};bool was_open{},was_closed{};
                    for(unsigned i=0;i<240u;++i){
                        machine.Advance(.1f);
                        const bool open=yaw()==target,close=yaw()==initial;
                        if(open && !was_open)++opened;if(close && !was_closed)++closed;
                        was_open=open;was_closed=close;
                        check(machine.TaskCount()<=6u,"Rotate rollback containers keep bounded task ownership");
                    }
                    check(opened>=2u && closed>=2u,"reverse Rotate/Start/helper complete multiple cycles");
                    (void)object.SetVisible(0u);
                    check(!machine.TaskCount() && !memory.ReadDword(owner+0x27cu),"visibility cancellation retires primary and reverse rotations");
                }
            }
            auto sound_after=sound.SampleHandles();std::sort(sound_after.begin(),sound_after.end());
            check(sound_after==sound_before,"Rotate owns and retires its sound handle without touching other samples");
            check(memory.AllocationAddresses()==allocations,"Rotate tasks/strings/helpers/containers restore exact allocation set");
        }
    }
    std::cout<<"Native M08 Rotate: actual door01/door02 Start16/Rotate1, three-second exact yaw, WC001 Miles ownership, activity, reverse cycles and allocation-set cleanup pass\n";
}
}
