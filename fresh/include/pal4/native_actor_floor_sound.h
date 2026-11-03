#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <string_view>
namespace pal4 {
struct PalActorControlState;
struct PalFloorAttributeState;
class NativeBaseObjectMemory;
class NativeEffectSceneSession;
struct NativeActorFloorSoundServices {
    std::function<void(std::string_view,const std::array<float,3>&,std::uint32_t,float,float)> play;
    std::function<void(std::string_view)> effect;
};
// Borrow scene services only during the original FL/FR callback.
std::uint32_t TriggerNativeActorFloorSound(PalActorControlState&,const PalFloorAttributeState&,
    NativeBaseObjectMemory&,std::uint32_t node,const NativeActorFloorSoundServices&);
void ApplyNativeActorFloorEffect(PalActorControlState&,NativeEffectSceneSession&,std::string_view);
}
