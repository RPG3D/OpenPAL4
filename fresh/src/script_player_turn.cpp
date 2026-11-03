#include "pal4/script_player_turn.h"

#include <cmath>
#include <string>

namespace pal4 {
namespace {

float TurnDelta(float current, float target) {
    const auto direct = target - current;
    // IDA5E1509/151C: this branch is current<180 AND target>180.
    // Keep the original two float operations before negation (5E1536..1543).
    if (current < 180.0f && target > 180.0f)
        return direct > 180.0f ? -(360.0f - target + current) : direct;
    if (current > 180.0f && target < 180.0f)
        return current - target > 180.0f ?
            360.0f - current + target : direct;
    return direct;
}

}  // namespace

std::uint32_t ScriptPlayerTurn::Start(
    std::int32_t index, float target_yaw, std::uint8_t wait_mode) {
    const auto manager = runtime_.PlayerManager();
    const auto actor = runtime_.PlayerByIndex(manager, index);
    return StartActor(actor, std::to_string(index) + "-Player-Movment",
                      target_yaw, wait_mode);
}

std::uint32_t ScriptPlayerTurn::FacePlayer(
    std::int32_t source_index, std::int32_t target_index,
    std::uint8_t wait_mode) {
    const auto source_manager = runtime_.PlayerManager();
    const auto source = runtime_.PlayerByIndex(source_manager, source_index);
    const auto target_manager = runtime_.PlayerManager();
    const auto target = runtime_.PlayerByIndex(target_manager, target_index);
    return FaceActor(source, target,
                     std::to_string(source_index) + "-Player-Movment",
                     wait_mode);
}

std::uint32_t ScriptPlayerTurn::FaceNpc(
    std::int32_t source_index, std::string_view npc_name,
    std::uint8_t wait_mode) {
    const auto manager = runtime_.PlayerManager();
    const auto source = runtime_.PlayerByIndex(manager, source_index);
    const auto npc_manager = runtime_.NpcManager();
    const auto target = runtime_.NpcByName(npc_manager, npc_name);
    return FaceActor(source, target,
                     std::to_string(source_index) + "-Player-Movment",
                     wait_mode);
}

std::uint32_t ScriptPlayerTurn::StartNpc(
    std::string_view npc_name, float target_yaw, std::uint8_t wait_mode) {
    const auto manager = runtime_.NpcManager();
    const auto actor = runtime_.NpcByName(manager, npc_name);
    return StartActor(actor, std::string(npc_name) + "-Player-Movment",
                      target_yaw, wait_mode);
}

std::uint32_t ScriptPlayerTurn::FaceNpcToNpc(
    std::string_view source_name, std::string_view target_name,
    std::uint8_t wait_mode) {
    const auto source_manager = runtime_.NpcManager();
    const auto source = runtime_.NpcByName(source_manager, source_name);
    const auto target_manager = runtime_.NpcManager();
    const auto target = runtime_.NpcByName(target_manager, target_name);
    if (source == 0u || target == 0u) return target;
    return FaceActor(source, target,
                     std::string(source_name) + "-Player-Movment",
                     wait_mode);
}

std::uint32_t ScriptPlayerTurn::FaceNpcToPlayer(
    std::string_view source_name, std::int32_t target_index,
    std::uint8_t wait_mode) {
    const auto manager = runtime_.NpcManager();
    const auto source = runtime_.NpcByName(manager, source_name);
    const auto player_manager = runtime_.PlayerManager();
    const auto target = runtime_.PlayerByIndex(player_manager, target_index);
    if (source == 0u || target == 0u) return target;
    return FaceActor(source, target,
                     std::string(source_name) + "-Player-Movment",
                     wait_mode);
}

std::uint32_t ScriptPlayerTurn::FacePlayerToCurrent(
    std::int32_t source_index, std::uint8_t wait_mode) {
    const auto source_manager = runtime_.PlayerManager();
    const auto source = runtime_.PlayerByIndex(source_manager, source_index);
    const auto target_manager = runtime_.PlayerManager();
    const auto target = runtime_.CurrentPlayer(target_manager);
    if (source == 0u || target == 0u) return target;
    return FaceActor(source, target,
                     std::to_string(source_index) + "-Player-Movment",
                     wait_mode);
}

std::uint32_t ScriptPlayerTurn::FaceCurrentToNpc(
    std::string_view target_name, std::uint8_t wait_mode) {
    const auto player_manager = runtime_.PlayerManager();
    const auto source = runtime_.CurrentPlayer(player_manager);
    const auto npc_manager = runtime_.NpcManager();
    const auto target = runtime_.NpcByName(npc_manager, target_name);
    if (source == 0u || target == 0u) return target;
    return FaceActor(source, target,
                     std::string(target_name) + "-Player-Movment",
                     wait_mode);
}

std::uint32_t ScriptPlayerTurn::FaceNpcToCurrent(
    std::string_view source_name, std::uint8_t wait_mode) {
    const auto npc_manager = runtime_.NpcManager();
    const auto source = runtime_.NpcByName(npc_manager, source_name);
    const auto player_manager = runtime_.PlayerManager();
    const auto target = runtime_.CurrentPlayer(player_manager);
    if (source == 0u || target == 0u) return target;
    return FaceActor(source, target,
                     std::string(source_name) + "-Player-Movment",
                     wait_mode);
}

std::uint32_t ScriptPlayerTurn::ResetNpcDirection(
    std::string_view npc_name) {
    const auto npc_manager = runtime_.NpcManager();
    const auto actor = runtime_.NpcByName(npc_manager, npc_name);
    if (actor == 0u) return 0u;
    const auto info_manager = runtime_.NpcManager();
    const auto info = runtime_.NpcInfoByName(info_manager, npc_name);
    if (info == 0u) return 0u;
    const auto target = runtime_.BaselineYaw(info);
    return StartActor(actor, std::string(npc_name) + "-Player-Movment",
                      target, 1u);
}

std::uint32_t ScriptPlayerTurn::FaceCurrentToObject(
    std::string_view object_name, std::uint8_t wait_mode) {
    const auto source = runtime_.CurrentPlayer(runtime_.PlayerManager());
    const auto target = runtime_.ObjectByName(runtime_.ObjectManager(),
                                              object_name);
    if (source == 0u || target == 0u) return target;

    const auto source_position = runtime_.ReadPosition(source);
    const auto target_position = runtime_.ReadPosition(target);
    PlayerTurnVector direction{
        target_position.x - source_position.x,
        target_position.y - source_position.y,
        target_position.z - source_position.z};
    runtime_.ClampFacingVector(direction);
    const auto angle = runtime_.FacingAngleDegrees(direction);
    const auto index = runtime_.CurrentPlayerIndex(runtime_.PlayerManager());
    return StartActor(source,
                      std::to_string(index) + "-Player-Movment",
                      angle, wait_mode);
}

std::uint32_t ScriptPlayerTurn::FaceActor(
    std::uint32_t source, std::uint32_t target,
    std::string_view name, std::uint8_t wait_mode) {
    if (source == 0u || target == 0u) return 0u;
    const auto source_position = runtime_.ReadPosition(source);
    const auto target_position = runtime_.ReadPosition(target);
    PlayerTurnVector direction{
        target_position.x - source_position.x,
        target_position.y - source_position.y,
        target_position.z - source_position.z};
    runtime_.ClampFacingVector(direction);
    return StartActor(source, name,
                      runtime_.FacingAngleDegrees(direction), wait_mode);
}

std::uint32_t ScriptPlayerTurn::StartActor(
    std::uint32_t actor, std::string_view name,
    float target_yaw, std::uint8_t wait_mode) {
    if (actor == 0u) return 0u;

    auto* state = runtime_.Allocate(sizeof(PlayerTurnWaitState));
    if (state != nullptr) {
        runtime_.Construct(*state, wait_mode,
                           runtime_.ScriptExecutor() + kQueueOffset);
        state->actor = actor;
        state->vtable_address = kVtable;
        runtime_.Flush(*state);
        state->phase = 2u;
        if (runtime_.AllocateText(*state,
                                  static_cast<std::uint32_t>(name.size())))
            runtime_.AssignName(*state, name);
        state->speed = kSpeed;
        state->start_yaw = runtime_.ReadYaw(actor);
        state->delta = target_yaw;
        state->duration = 0.0f;
        state->elapsed = 0.0f;
        if (std::fabs(state->start_yaw - target_yaw) >= 0.005f) {
            state->delta = TurnDelta(state->start_yaw, target_yaw);
            state->duration = static_cast<float>(
                std::fabs(static_cast<double>(state->delta)) *
                0.04761904761904762);
            if (runtime_.SetAction(actor, "C02") == 0)
                static_cast<void>(runtime_.SetAction(actor, "Z02"));
            runtime_.CleanupWeaponEffects(actor);
        } else {
            runtime_.Close(*state);
        }
    }
    runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset, state);
    return runtime_.CleanupName();
}

void ScriptPlayerTurn::Update(PlayerTurnWaitState& state, double elapsed) {
    if (state.duration <= 0.0f) {
        runtime_.Close(state);
        return;
    }
    state.elapsed = static_cast<float>(state.speed * elapsed + state.elapsed);
    if (state.elapsed >= state.duration) {
        state.elapsed = state.duration;
        RestoreIdle(state);
    }
    const auto fraction = static_cast<double>(state.elapsed) /
                          state.duration;
    const auto yaw = static_cast<float>(fraction * state.delta +
                                        state.start_yaw);
    const auto x = runtime_.ReadX(state.actor);
    const auto z = runtime_.ReadZ(state.actor);
    runtime_.SetRotation(state.actor, x, yaw, z, 0u);
}

void ScriptPlayerTurn::Finish(PlayerTurnWaitState& state) {
    RestoreIdle(state);
}

void ScriptPlayerTurn::RestoreIdle(PlayerTurnWaitState& state) {
    if (runtime_.SetTimedAction(state.actor, "C01", kBlend) == 0)
        static_cast<void>(runtime_.SetTimedAction(
            state.actor, "Z01", kBlend));
    runtime_.Close(state);
}

}  // namespace pal4
