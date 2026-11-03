#pragma once

#include "pal4/pal_actor_control.h"
#include "pal4/pal_actor_control_effect_selector.h"
#include "pal4/game_section_executor.h"
#include "pal4/pal_actor_control_game_event.h"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace pal4 {

struct ActorInteractionInput {
    std::int32_t blocking_count = 0;
    std::uint32_t blocking_mode = 0;
    std::int32_t primary_count = 0;
    std::uint32_t primary_mode = 0;
    std::uint32_t primary_release = 0;
    std::int32_t secondary_count = 0;
    std::uint32_t secondary_mode = 0;
    std::uint32_t secondary_release = 0;
};

struct ActorInteractionPoint {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

using ActorInteractionRay = std::array<float, 6>;

class IActorControlInteractionRuntime : public IGameSectionRuntime,
                                        public IActorControlGameEventRuntime {
public:
    virtual ~IActorControlInteractionRuntime() = default;
    virtual ActorInteractionInput Input();
    virtual ActorInteractionPoint CursorClient();
    virtual ActorInteractionRay PixelRay(
        std::uint32_t camera_internal, ActorInteractionPoint point);
    virtual std::uint32_t NpcByRay(const ActorInteractionRay& ray);
    virtual std::uint32_t DialogByRay(const ActorInteractionRay& ray);
    virtual std::uint32_t EffectByRay(
        std::uint32_t camera_internal, ActorInteractionPoint point);
    virtual std::uint32_t NpcByPosition(std::uint32_t actor_position);
    virtual std::uint32_t DialogByPosition(
        std::uint32_t actor_position, float actor_property);
    virtual std::vector<std::uint32_t> EffectCandidates();
    virtual std::uint32_t ReadWord(std::uint32_t address);
    virtual float ReadFloat(std::uint32_t address);
    virtual double ActorProperty(std::uint32_t actor);
    virtual ActorControlEffectPoint EffectVertex(
        std::uint32_t effect, std::int32_t index);
    virtual float PlanarMagnitude(ActorControlEffectPoint offset);
    virtual double SpatialMagnitude(ActorControlEffectPoint offset);
    virtual void SetCursor(std::string_view resource);
    virtual double NpcDistance(PalActorControlState& state,
                               std::uint32_t npc_position);
    virtual void ExecuteScript(std::uint32_t script);
    virtual std::string ReadCString(std::uint32_t address);
    virtual bool DialogEnabled(std::uint32_t dialog);
    virtual bool DialogInRange(std::uint32_t dialog,
                               std::uint32_t actor_position,
                               float actor_property);
    virtual bool ProcessDialogEvent(PalActorControlState& state,
                                    std::uint32_t dialog);
    virtual void AttachDialog(std::uint32_t dialog,
                              std::uint32_t actor_position);
    virtual void TriggerEffect(std::uint32_t effect_action);
};

class PalActorControlInteraction final {
public:
    // IDA 0x425430. The three active target fields are words 20-22.
    [[nodiscard]] static bool Handle(
        PalActorControlState& state, std::uint32_t camera_internal,
        IActorControlInteractionRuntime& runtime);
    [[nodiscard]] static double DistanceToNpc(
        PalActorControlState& state, std::uint32_t npc_position,
        IActorControlInteractionRuntime& runtime);
};

}  // namespace pal4
