#include "pal4/script_player_rotation.h"

#include <string>

namespace pal4 {
namespace {

float DirectionAngle(std::string_view direction) {
    if (direction == "N") return 0.0f;
    if (direction == "S") return 180.0f;
    if (direction == "W") return 90.0f;
    if (direction == "E") return 270.0f;
    if (direction == "NW") return 45.0f;
    if (direction == "NE") return 315.0f;
    if (direction == "SE") return 225.0f;
    if (direction == "SW") return 135.0f;
    return 0.0f;
}

float NpcDirectionAngle(std::string_view direction) {
    if (direction == "N") return 0.0f;
    if (direction == "S") return 180.0f;
    if (direction == "W") return 90.0f;
    if (direction == "E") return 270.0f;
    if (direction == "WN") return 45.0f;
    if (direction == "EN") return 315.0f;
    if (direction == "ES") return 225.0f;
    if (direction == "WS") return 135.0f;
    return 0.0f;
}

}  // namespace

std::uint32_t ScriptPlayerRotation::SetDirection(
    std::int32_t index, std::string_view direction) {
    const auto manager = runtime_.PlayerManager();
    return SetActor(runtime_.PlayerByIndex(manager, index),
                    DirectionAngle(direction), index, false, false, {});
}

std::uint32_t ScriptPlayerRotation::SetAngle(
    std::int32_t index, float angle) {
    const auto manager = runtime_.PlayerManager();
    return SetActor(runtime_.PlayerByIndex(manager, index),
                    angle, index, false, false, {});
}

std::uint32_t ScriptPlayerRotation::SetCurrentAngle(float angle) {
    return SetActor(runtime_.CurrentPlayer(runtime_.PlayerManager()),
                    angle, 0, true, false, {});
}

std::uint32_t ScriptPlayerRotation::SetNpcDirection(
    std::string_view npc_name, std::string_view direction) {
    const auto actor = runtime_.NpcByName(runtime_.NpcManager(), npc_name);
    return SetActor(actor, NpcDirectionAngle(direction), 0, false,
                    true, npc_name);
}

std::uint32_t ScriptPlayerRotation::SetNpcAngle(
    std::string_view npc_name, float angle) {
    const auto actor = runtime_.NpcByName(runtime_.NpcManager(), npc_name);
    return SetActor(actor, angle, 0, false, true, npc_name);
}

std::uint32_t ScriptPlayerRotation::SetActor(
    std::uint32_t actor, float angle, std::int32_t index,
    bool current, bool npc, std::string_view npc_name) {
    if (actor == 0u) return 0u;
    const PlayerRotation rotation{runtime_.ReadX(actor), angle,
                                  runtime_.ReadZ(actor)};
    runtime_.SetRotation(actor, rotation, 0u);

    const auto label = npc ? std::string(npc_name) :
                       current ? std::string(runtime_.ActorName(actor)) :
                                 std::to_string(index);
    const auto name = label + "-Player-Movment";
    auto* state = runtime_.Allocate(sizeof(PlayerRotationWaitState));
    if (state != nullptr) {
        runtime_.Construct(*state, 0u,
                           runtime_.ScriptExecutor() + kQueueOffset);
        state->phase = 2u;
        state->vtable_address = kVtable;
        state->enabled = 1u;
        if (runtime_.AllocateText(*state,
                                  static_cast<std::uint32_t>(name.size())))
            runtime_.AssignName(*state, name);
        runtime_.Close(*state);
    }
    runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset, state);
    if (state && completed_handlers_)
        completed_handlers_->AppendCompletedHandler(kVtable, 2, name);
    return runtime_.CleanupName();
}

}  // namespace pal4
