#pragma once

#include "pal4/actor_control_input_component.h"

#include <cstdint>

namespace pal4 {

struct ActorControlScreenPoint {
    std::int32_t x{};
    std::int32_t y{};
};

struct ActorControlScreenRect {
    std::int32_t left{};
    std::int32_t top{};
    std::int32_t right{};
    std::int32_t bottom{};
};

class IActorControlInputEventRuntime {
public:
    virtual ~IActorControlInputEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InputManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadInputWord(
        std::uint32_t manager, std::uint32_t offset) = 0;
    [[nodiscard]] virtual ActorControlScreenPoint CursorPoint() = 0;
    [[nodiscard]] virtual ActorControlScreenRect WindowRect() = 0;
    virtual void GetInputStateValues(std::uint32_t manager,
                                     std::uint32_t& input_bits,
                                     std::uint32_t& scratch_actor) = 0;
    virtual void DispatchMovement(ActorControlInputComponentState& state,
                                  std::uint32_t actor, std::uint32_t command,
                                  float delta_seconds) = 0;
    virtual void TurnFromMouse(ActorControlInputComponentState& state,
                               std::uint32_t actor,
                               std::int32_t input_value) = 0;
};

class ActorControlInputEventRuntimeStub final
    : public IActorControlInputEventRuntime {
public:
    std::uint32_t InputManager() override;
    std::uint32_t ReadInputWord(std::uint32_t manager,
                                std::uint32_t offset) override;
    ActorControlScreenPoint CursorPoint() override;
    ActorControlScreenRect WindowRect() override;
    void GetInputStateValues(std::uint32_t manager,
                             std::uint32_t& input_bits,
                             std::uint32_t& scratch_actor) override;
    void DispatchMovement(ActorControlInputComponentState& state,
                          std::uint32_t actor, std::uint32_t command,
                          float delta_seconds) override;
    void TurnFromMouse(ActorControlInputComponentState& state,
                       std::uint32_t actor,
                       std::int32_t input_value) override;
};

}  // namespace pal4
