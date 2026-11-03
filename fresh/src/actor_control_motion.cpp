#include "pal4/actor_control_motion.h"

#include <bit>
#include <cmath>

namespace pal4 {

namespace {
MotionVector Decode(const std::array<std::uint32_t, 3>& raw) {
    return {std::bit_cast<float>(raw[0]),
            std::bit_cast<float>(raw[1]),
            std::bit_cast<float>(raw[2])};
}

MotionVector ReadActorVector(std::uint32_t actor, std::uint32_t offset,
                             IActorControlMotionRuntime& runtime) {
    return Decode({runtime.ReadDword(actor + offset),
                   runtime.ReadDword(actor + offset + 4u),
                   runtime.ReadDword(actor + offset + 8u)});
}
}  // namespace

std::uint32_t ActorControlMotionRuntimeStub::FreeAddress(
    ActorControlMotionState*) { return 0u; }
std::uint32_t ActorControlMotionRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
MotionVector ActorControlMotionRuntimeStub::ConvertPosition(
    const std::array<std::uint32_t, 3>& raw) { return Decode(raw); }
bool ActorControlMotionRuntimeStub::PositionsDiffer(
    const MotionVector& left, const MotionVector& right) {
    return left != right;
}
std::uint32_t ActorControlMotionRuntimeStub::GameArenaManager() { return 0u; }
std::uint32_t ActorControlMotionRuntimeStub::CollisionProbe(
    std::uint32_t, const MotionVector&) { return 0u; }
bool ActorControlMotionRuntimeStub::TryMove(
    std::uint32_t, const MotionVector&, const MotionVector&,
    std::uint32_t, std::uint32_t) { return false; }
void ActorControlMotionRuntimeStub::SetMovementSpeed(
    std::uint32_t, std::uint32_t) {}
void ActorControlMotionRuntimeStub::SetMovementTarget(
    std::uint32_t, const MotionVector&) {}
void ActorControlMotionRuntimeStub::ActorCommand(
    std::uint32_t, const char*) {}
bool ActorControlMotionRuntimeStub::CollisionBlocked(std::uint32_t) {
    return false;
}
void ActorControlMotionRuntimeStub::SnapActorPosition(
    std::uint32_t, const MotionVector&, std::uint32_t) {}
float ActorControlMotionRuntimeStub::VectorMagnitude(
    const MotionVector& delta) {
    return std::sqrt(delta[0] * delta[0] + delta[1] * delta[1] +
                     delta[2] * delta[2]);
}
std::uint32_t ActorControlMotionRuntimeStub::InitializeActorControl() {
    return 0u;
}
void ActorControlMotionRuntimeStub::ResetActorControl(std::uint32_t) {}
std::uint32_t ActorControlMotionRuntimeStub::UiFrameManager() { return 0u; }
void ActorControlMotionRuntimeStub::ProcessUiEvents(
    std::uint32_t, std::uint32_t) {}
std::uint32_t ActorControlMotionRuntimeStub::MonsterManager() { return 0u; }
void ActorControlMotionRuntimeStub::ClearMonsterFlags(std::uint32_t) {}
void ActorControlMotionRuntimeStub::UpdateActorPosition(
    std::uint32_t, const MotionVector&) {}
void ActorControlMotionRuntimeStub::NormalizeDirection(
    MotionVector& direction) {
    const auto length = VectorMagnitude(direction);
    if (length != 0.0f)
        for (auto& component : direction) component /= length;
}
std::uint32_t ActorControlMotionRuntimeStub::SetMonsterFlags(
    std::uint32_t) { return 0u; }

ActorControlMotionState& ActorControlMotion::Construct(
    ActorControlMotionState& state) {
    state.vtable = kVtable;
    state.reserved08 = 0u;
    state.active = 0u;
    state.mode = 0u;
    state.pending = 0u;
    state.reached = 1u;
    state.speed_bits = kInitialSpeedBits;
    return state;
}

ActorControlMotionState* ActorControlMotion::DeleteBase(
    ActorControlMotionState* state, std::uint32_t flags,
    IActorControlMotionRuntime& runtime) {
    state->vtable = kBaseVtable;
    if ((flags & 1u) != 0u)
        static_cast<void>(runtime.FreeAddress(state));
    return state;
}

std::uint32_t ActorControlMotion::Reset(
    ActorControlMotionState& state) {
    state.reserved08 = 0u;
    state.active = 0u;
    state.mode = 0u;
    state.pending = 0u;
    state.reached = 1u;
    return 1u;
}

std::uint32_t ActorControlMotion::ResetWithArgument(
    ActorControlMotionState& state, std::uint32_t) {
    return Reset(state);
}

std::uint32_t ActorControlMotion::Update(
    ActorControlMotionState& state, std::uint32_t actor,
    float delta_seconds, IActorControlMotionRuntime& runtime) {
    if (state.active == 0u) return state.active;
    const auto collision = actor + 1380u;
    if (state.mode != 0u) {
        if (state.pending != 0u) {
            const auto current = runtime.ConvertPosition({
                runtime.ReadDword(actor + 172u),
                runtime.ReadDword(actor + 176u),
                runtime.ReadDword(actor + 180u)});
            const auto target = runtime.ConvertPosition(state.target_position);
            if (runtime.PositionsDiffer(current, target)) {
                const auto arena = runtime.ReadDword(
                    runtime.GameArenaManager() + 404u);
                const auto probe = runtime.CollisionProbe(arena, target);
                if (probe != 0u &&
                    runtime.TryMove(collision, current, target, probe, 0u)) {
                    runtime.SetMovementSpeed(collision, 0x42340000u);
                    runtime.SetMovementTarget(collision, target);
                    runtime.ActorCommand(actor + 368u, "C02");
                }
            }
            state.pending = 0u;
            return state.active;
        }
        if (runtime.CollisionBlocked(collision)) return state.active;
        state.mode = 0u;
        auto position = ReadActorVector(actor, 160u, runtime);
        position[1] = std::bit_cast<float>(state.alternate_height_bits);
        runtime.SnapActorPosition(actor, position, 0u);
        runtime.ActorCommand(actor + 368u,
                             state.reached != 0u ? "C04" : "C05");
        return state.active;
    }

    const auto position = ReadActorVector(actor, 172u, runtime);
    const auto direction = Decode(state.direction);
    const auto step = delta_seconds *
                      std::bit_cast<float>(state.speed_bits);
    MotionVector candidate{
        position[0] + direction[0] * step,
        position[1] + direction[1] * step,
        position[2] + direction[2] * step};
    const auto target = Decode(state.target_position);
    const MotionVector delta{candidate[0] - target[0],
                              candidate[1] - target[1],
                              candidate[2] - target[2]};
    const auto magnitude = runtime.VectorMagnitude(delta);
    if (magnitude >= std::bit_cast<float>(state.threshold_bits)) {
        state.active = 0u;
        state.mode = 0u;
        state.pending = 0u;
        state.reached = 1u;
        candidate = Decode(state.final_position);
        runtime.ResetActorControl(runtime.InitializeActorControl());
        runtime.ProcessUiEvents(runtime.UiFrameManager(), 1u);
        runtime.ClearMonsterFlags(runtime.MonsterManager());
    }
    runtime.UpdateActorPosition(actor, candidate);
    return state.active;
}

std::uint32_t ActorControlMotion::Begin(
    ActorControlMotionState& state,
    const std::array<std::uint32_t, 3>& start,
    const std::array<std::uint32_t, 3>& finish,
    std::uint32_t alternate_height_bits, std::uint32_t reached,
    IActorControlMotionRuntime& runtime) {
    state.target_position = start;
    state.final_position = finish;
    const auto first = Decode(start);
    const auto last = Decode(finish);
    MotionVector direction{last[0] - first[0],
                            last[1] - first[1],
                            last[2] - first[2]};
    state.active = 1u;
    state.mode = 1u;
    state.pending = 1u;
    state.reached = reached;
    const auto distance = runtime.VectorMagnitude(direction);
    state.threshold_bits = std::bit_cast<std::uint32_t>(distance);
    runtime.NormalizeDirection(direction);
    for (std::size_t i = 0u; i < 3u; ++i)
        state.direction[i] = std::bit_cast<std::uint32_t>(direction[i]);
    state.alternate_height_bits = alternate_height_bits;
    runtime.ProcessUiEvents(runtime.UiFrameManager(), 0u);
    return runtime.SetMonsterFlags(runtime.MonsterManager());
}

}  // namespace pal4
