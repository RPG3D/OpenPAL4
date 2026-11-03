#pragma once
#include "pal4/pal_game_object_factory_object.h"
#include <functional>
namespace pal4 {
class NativeScenePlayerMovement;
class NativeActorControlSession;
// Borrows the current movement and ActorControl owners for EVF enter/leave.
class NativeSceneEvfControlRuntime final : public IPalGameObjectFactoryControlRuntime {
public:
    NativeSceneEvfControlRuntime(NativeScenePlayerMovement& movement,
        NativeActorControlSession& actor,std::function<void(std::uint32_t)> display);
    std::uint32_t MovementAdapter() override;
    std::uint32_t ActorControl() override;
    void WriteWord(std::uint32_t address,std::uint32_t value) override;
    void InvokeActorControlProperty(std::uint32_t object,
        std::uint32_t virtual_offset,std::uint32_t value) override;
private:
    NativeScenePlayerMovement& movement_;
    NativeActorControlSession& actor_;
    std::function<void(std::uint32_t)> display_;
};
}
