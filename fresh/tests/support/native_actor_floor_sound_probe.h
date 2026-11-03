#pragma once
#include "pal4/native_actor_floor_sound.h"
#include "pal4/native_actor_animation_state_binding.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_miles_sound_3d_runtime.h"
#include "pal4/game_object_animation_event_data.h"
#include "pal4/game_object_embedded_animation.h"
#include "pal4/pal_actor_control.h"
#include "pal4/pal_floor_attribute.h"
#include <bit>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace pal4::test {
inline void VerifyNativeActorFloorSound(PalActorControlState& control,
    NativeActorAnimationStateBinding& binding,NativeBaseObjectMemory& memory,
    NativeMilesSound3dRuntime& sounds) {
    const auto check=[](bool ok){if(!ok)throw std::runtime_error("native FL/FR floor sound regression");};
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    const auto actor=control.words[1u];check(actor && binding.Select("C01"));
    auto& animation=*reinterpret_cast<GameObjectEmbeddedAnimationState*>(actor+0x170u);
    const auto record=animation.words[3u];
    const std::array<std::uint32_t,3> saved_events{memory.ReadDword(record+40u),memory.ReadDword(record+44u),memory.ReadDword(record+48u)};
    const auto ready=control.words[0u];const auto left=address(control.words.data()+209u),right=address(control.words.data()+216u);
    const auto old_callback=memory.ReadDword(right+24u),old_owner=memory.ReadDword(right+20u);
    std::vector<std::uint32_t> voices;std::vector<std::string> order;
    PalFloorAttributeState floor{};const char sound[]="WF036.wav";
    floor.text={0u,address(sound),sizeof(sound)-1u,sizeof(sound)-1u};
    NativeActorFloorSoundServices services;
    services.play=[&](std::string_view path,const auto& position,std::uint32_t loops,float maximum,float minimum) {
        check(path==sound && loops==1u && maximum==600.f && minimum==300.f);
        check(position==*reinterpret_cast<const std::array<float,3>*>(actor+172u));
        const auto id=sounds.PlayNamedSound(path,position,loops,maximum,minimum);voices.push_back(id);
        const auto state=sounds.Inspect(id);check(state.loop_count==1u && state.distances==std::array<float,2>{600.f,300.f});
        check(state.miles_position==std::array<float,3>{position[0],position[1],-position[2]});
        order.emplace_back("sound");
    };
    services.effect=[&](std::string_view path){check(path==sound);order.emplace_back("effect");};
    const auto restore=[&] {
        binding.BindFloorSound({});
        for(auto id:voices)(void)sounds.Stop(id);
        for(unsigned i=0;i<3u;++i)memory.WriteDword(record+40u+4u*i,saved_events[i]);
        memory.WriteDword(right+24u,old_callback);memory.WriteDword(right+20u,old_owner);control.words[0u]=ready;
        check(binding.Select("C01"));
    };
    try {
        // Existing named nodes must receive both original AMF messages through
        // the shared event dispatcher, setting the selected-listener field.
        std::array<AnimationEventData,2> events{};
        std::memcpy(events[0].data()+128u,"fl",3u);std::memcpy(events[1].data()+128u,"FR",3u);
        const auto begin=address(events.data());
        memory.WriteDword(record+40u,begin);memory.WriteDword(record+44u,begin+sizeof(events));memory.WriteDword(record+48u,begin+sizeof(events));
        binding.BindFloorSound([&](std::uint32_t node){check(TriggerNativeActorFloorSound(control,floor,memory,node,services)==1u);});
        control.words[0u]=1u;
        check(binding.AdvanceFrame(0.01f));
        check(binding.SelectedListenerName()=="FR" && animation.words[10u]==right && order==std::vector<std::string>{"sound","effect","sound","effect"});
        check(memory.ReadDword(begin+196u)==1u && memory.ReadDword(begin+sizeof(AnimationEventData)+196u)==1u);
        const auto count=order.size();
        floor.text={};check(TriggerNativeActorFloorSound(control,floor,memory,left,services)==1u && order.size()==count);
        floor.text={0u,address(sound),sizeof(sound)-1u,sizeof(sound)-1u};
        control.words[0u]=0u;check(TriggerNativeActorFloorSound(control,floor,memory,left,services)==0u && order.size()==count);
        control.words[0u]=1u;memory.WriteDword(right+20u,0u);
        check(TriggerNativeActorFloorSound(control,floor,memory,right,services)==1u && order.size()==count);
        memory.WriteDword(right+20u,old_owner);memory.WriteDword(right+24u,0u);
        check(TriggerNativeActorFloorSound(control,floor,memory,right,services)==1u && order.size()==count+1u && order.back()=="sound");
        bool foreign{};try{(void)TriggerNativeActorFloorSound(control,floor,memory,actor,services);}catch(const std::invalid_argument&){foreign=true;}
        check(foreign);
        binding.BindFloorSound({});binding.ResetRecordEvents(record);
        check(binding.Select("C01") && binding.AdvanceFrame(0.01f) && animation.words[10u]==0u && order.size()==count+1u);
    } catch(...) {restore();throw;}
    restore();
    std::cout<<"Native FL/FR: real animation messages select nodes, actual Miles3D position/range/loop, callback order, empty/inactive/missing-owner/optional/unbind and cleanup verified\n";
}
}
