#pragma once
#include "pal4/native_scene_evf_assets.h"
#include "pal4/native_scene_evf_selection.h"
#include "pal4/native_scene_movement_asset.h"
#include "pal4/native_scene_player_movement.h"
#include "pal4/native_scene_evf_control_runtime.h"
#include "pal4/native_scene_evf_event_callbacks.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <bit>
namespace pal4::test {
inline void VerifyNativeGuestSoundControls(const std::filesystem::path& root,
    NativeActorControlSession& actor,NativeNewGameScriptSession& scripts) {
    const auto require=[](bool value,const char* message){if(!value)throw std::runtime_error(message);};
    NativeSceneEvfAssets empty;
    require(NativeSceneEvfSelection(empty).Records().empty(),"scene without EVF has no interaction candidates");
    NativeSceneMovementAsset scene;
    NativeSceneEvfAssets events;
    require(scene.Load(root,"Q03","XN03") && events.Load(root,"Q03","XN03"),
        "guest sound probe loads original movement and EVF");
    NativeScenePlayerMovement movement(scene);
    std::vector<std::uint32_t> display;
    NativeSceneEvfControlRuntime controls(movement,actor,[&](auto value){display.push_back(value);});
    NativeSceneEvfEventCallbacks callbacks(events,scene,scripts,&controls);
    const NativeSceneEvfRecord* sound{};
    events.VisitRecords([&](auto record){
        const auto& state=*reinterpret_cast<const SceneEvfRecordState*>(record);
        if(state.words[1]==3u)sound=events.FindByResource(state.words[16]);
        return true;
    });
    require(sound && sound->effect_type==3u && sound->clump,"original guest room contains sound event");
    NativeSceneEvfSelection selection(events);
    const auto records=selection.Records();
    require(records.size()==events.Size() && records.size()>=2u,"real guest EVF records are available to keyboard selection");
    const NativeSceneEvfRecord* bed{};
    for(const auto record:records){
        const auto& state=*reinterpret_cast<const SceneEvfRecordState*>(record);
        const auto* owned=events.FindByResource(state.words[16]);
        if(owned && owned->script_function=="func2013")bed=owned;
    }
    require(bed && bed->properties.size()==8u,"original bed action owns eight interaction properties");
    for(std::size_t index=0;index<bed->properties.size();++index){
        const auto point=selection.PropertyPoint(bed->RecordAddress(),static_cast<std::int32_t>(index));
        for(unsigned axis=0;axis<3;++axis)
            require(std::bit_cast<std::uint32_t>(point[axis])==bed->properties[index].words[axis],
                "bed property coordinates come from the original80-byte property array");
    }
    bool foreign=false,range=false;
    try{(void)selection.PropertyPoint(bed->RecordAddress()+4u,0);}catch(const std::invalid_argument&){foreign=true;}
    try{(void)selection.PropertyPoint(bed->RecordAddress(),8);}catch(const std::out_of_range&){range=true;}
    require(foreign && range,"EVF selection rejects foreign records and properties outside their owner");
    const auto resource=sound->GeometryState().words[16];
    require(scene.RegisterEventClump(sound->clump,resource,false),"guest sound event registers real geometry");
    struct Unregister {
        NativeSceneMovementAsset& scene;RpClump* clump;
        ~Unregister(){(void)scene.UnregisterEventClump(clump,false);}
    } cleanup{scene,sound->clump};
    auto& state=*reinterpret_cast<const PalMovementAdapterState*>(movement.Address());
    callbacks.Enter(resource,0u);
    require(state.words[4]==1u,"sound enter changes actual current movement adapter");
    callbacks.Leave(resource);
    require(state.words[4]==0u,"sound leave clears actual movement adapter flag");
    (void)PalGameObjectFactoryObject::SetActorPropertyZero(0u,controls);
    (void)PalGameObjectFactoryObject::SetActorPropertyOne(controls);
    require(display==std::vector<std::uint32_t>{0u,1u},"local float virtual dispatch reaches bound display endpoint");
    bool rejected=false;
    try{controls.WriteWord(movement.Address()+12u,1u);}catch(const std::invalid_argument&){rejected=true;}
    require(rejected,"EVF control rejects writes outside its movement flag");
    std::cout<<"Native guest EVF control: original sound enter/leave live movement1/0, local float dispatch0/1 and ownership guard, real bed candidates/eight property coordinates pass\n";
}
}
