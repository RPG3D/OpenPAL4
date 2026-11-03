#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct ActorControlInputComponentState {
    std::array<std::uint32_t, 12> words{};
};
static_assert(sizeof(ActorControlInputComponentState) == 0x30u);

struct ActorControlMovementGlobals;
class IActorControlDiscreteMovementRuntime;
class IActorControlInputEventRuntime;
class IActorControlFreeMovementRuntime;

class IActorControlInputComponentRuntime {
public:
    virtual ~IActorControlInputComponentRuntime() = default;
    [[nodiscard]] virtual std::int32_t GameConfigPrimary() = 0;
    virtual void SetAnimationFlag(std::uint32_t animation,
                                  std::uint32_t value) = 0;
    virtual void HandleInputEvents(ActorControlInputComponentState& state,
                                   std::uint32_t actor, float delta_seconds) = 0;
    virtual void UpdateInputObject(ActorControlInputComponentState& state,
                                   std::uint32_t actor, float delta_seconds) = 0;
};

class ActorControlInputComponentRuntimeStub final
    : public IActorControlInputComponentRuntime {
public:
    [[nodiscard]] std::int32_t GameConfigPrimary() override;
    void SetAnimationFlag(std::uint32_t animation,
                          std::uint32_t value) override;
    void HandleInputEvents(ActorControlInputComponentState& state,
                           std::uint32_t actor, float delta_seconds) override;
    void UpdateInputObject(ActorControlInputComponentState& state,
                           std::uint32_t actor, float delta_seconds) override;
};

class ActorControlInputComponent final {
public:
    static constexpr std::uint32_t kVtable = 0x842700u;

    [[nodiscard]] static ActorControlInputComponentState& Construct(
        ActorControlInputComponentState& state,
        IActorControlInputComponentRuntime& runtime);
    [[nodiscard]] static std::uint32_t Initialize(
        ActorControlInputComponentState& state);
    [[nodiscard]] static std::uint32_t ResetActor(
        ActorControlInputComponentState& state, std::uint32_t actor,
        IActorControlInputComponentRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetActorWithoutAnimation(
        ActorControlInputComponentState& state, std::uint32_t actor);
    [[nodiscard]] static std::uint32_t ProcessInput(
        ActorControlInputComponentState& state, std::uint32_t actor,
        float delta_seconds, IActorControlInputComponentRuntime& runtime);
    [[nodiscard]] static std::uint32_t DispatchDiscreteMovement(
        ActorControlInputComponentState& state,
        ActorControlMovementGlobals& globals, std::uint32_t actor,
        std::uint32_t command, float delta_seconds,
        IActorControlDiscreteMovementRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyMouseTurn(
        ActorControlInputComponentState& state, std::uint32_t actor,
        std::uint32_t input_value_address, std::uint32_t unused,
        IActorControlDiscreteMovementRuntime& runtime);
    [[nodiscard]] static std::uint32_t DispatchVectorMovement(
        ActorControlInputComponentState& state,
        ActorControlMovementGlobals& globals, std::uint32_t actor,
        std::uint32_t direction_address, float delta_seconds,
        IActorControlDiscreteMovementRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetMovementMode(
        ActorControlInputComponentState& state, std::uint32_t mode,
        std::uint32_t unused);
    [[nodiscard]] static std::uint32_t HandleInputEvents(
        ActorControlInputComponentState& state, std::uint32_t actor,
        float delta_seconds, IActorControlInputEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyMouseTurnValue(
        ActorControlInputComponentState& state, std::uint32_t actor,
        std::int32_t input_value, IActorControlDiscreteMovementRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateFreeMovement(
        ActorControlInputComponentState& state, std::uint32_t actor,
        float delta_seconds, IActorControlFreeMovementRuntime& runtime);
    [[nodiscard]] static bool HasMovementInput(
        const ActorControlInputComponentState& state,
        IActorControlFreeMovementRuntime& runtime);
};

}  // namespace pal4
