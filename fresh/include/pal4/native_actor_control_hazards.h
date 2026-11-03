#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <memory>

namespace pal4 {
struct PalActorControlState;
struct PalFloorAttributeState;
class NativePlayerManagerSession;
class NativeBaseObjectMemory;
class NativeCameraManagerOwner;
class NativeEffectSceneSession;
class IUiTextImageOverlayRuntime;
struct NativeActorHazardUiServices {
    std::function<std::uint32_t()> manager,portrait;
    std::function<void()> update_portrait;
    std::function<void(std::uint32_t)> set_mode;
    std::function<std::array<float,2>(const std::array<float,2>&)> coordinates;
};
// Short-lived execution adapter. Persistent status words belong to ActorControl,
// feedback to each Player's existing queue, effects to the creating arena group.
class NativeActorControlHazards final {
public:
    NativeActorControlHazards(PalActorControlState&,NativePlayerManagerSession&,
        NativeBaseObjectMemory&,NativeCameraManagerOwner&,IUiTextImageOverlayRuntime&,
        NativeEffectSceneSession&,const PalFloorAttributeState&,NativeActorHazardUiServices);
    ~NativeActorControlHazards();
    void ApplyStatus(float percentage);
    void StartStatus(std::uint32_t duration,std::uint32_t interval,float percentage);
    void BeginMovement(std::uint32_t duration);
    void Tick(float elapsed);
    void TickFloorSound(float elapsed);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
