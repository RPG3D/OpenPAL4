#include "pal4/actor_control_update.h"
#include "pal4/game_vector_math.h"

#include <bit>
#include <cmath>
#include <limits>
#include <string>

namespace pal4 {

namespace {
float ReadFloat(std::uint32_t word) {
    return std::bit_cast<float>(word);
}

void WriteFloat(std::uint32_t& word, float value) {
    word = std::bit_cast<std::uint32_t>(value);
}

ActorControlVector ReadPosition(const ActorControlPlayerPrefix& player) {
    return {ReadFloat(player.words[43]), ReadFloat(player.words[44]),
            ReadFloat(player.words[45])};
}

ActorControlVector ReadStatePosition(const ActorControlDatabasePrefix& state,
                                      std::size_t start) {
    return {ReadFloat(state.words[start]), ReadFloat(state.words[start + 1u]),
            ReadFloat(state.words[start + 2u])};
}

void WritePosition(ActorControlDatabasePrefix& state, std::size_t start,
                   const ActorControlVector& position) {
    for (std::size_t index = 0; index < 3u; ++index)
        WriteFloat(state.words[start + index], position[index]);
}

ActorControlVector Difference(const ActorControlVector& left,
                               const ActorControlVector& right) {
    return {left[0] - right[0], left[1] - right[1],
            left[2] - right[2]};
}
}  // namespace

ActorControlVector ActorControlUpdateRuntimeStub::PlayerForward(
    const ActorControlPlayerPrefix&) { return {}; }
double ActorControlUpdateRuntimeStub::PlayerProperty(
    const ActorControlPlayerPrefix&) { return 0.0; }
bool ActorControlUpdateRuntimeStub::InteractionBlocked(
    ActorControlDatabasePrefix&) { return false; }
bool ActorControlUpdateRuntimeStub::ProgressVisible() { return false; }
double ActorControlUpdateRuntimeStub::ProgressValue() { return 0.0; }
void ActorControlUpdateRuntimeStub::SetProgressVisible(bool) {}
ActorControlInput ActorControlUpdateRuntimeStub::ReadInput() { return {}; }
void ActorControlUpdateRuntimeStub::Normalize(ActorControlVector& value) {
    const auto length = VectorLength(value);
    if (length != 0.0f)
        for (auto& coordinate : value) coordinate /= length;
}
ActorControlGroundResult ActorControlUpdateRuntimeStub::FindGround(
    const ActorControlVector&, float, float) { return {}; }
ActorControlGroundResult ActorControlUpdateRuntimeStub::CheckGround(
    const ActorControlVector&, float, float) { return {}; }
void ActorControlUpdateRuntimeStub::SetResourcePosition(
    ActorControlDatabasePrefix&, const ActorControlVector&,
    std::uint32_t) {}
void ActorControlUpdateRuntimeStub::MoveResource(
    ActorControlDatabasePrefix&, const ActorControlVector&,
    std::uint32_t) {}
std::uint32_t ActorControlUpdateRuntimeStub::HAnimFrameId(std::uint32_t) {
    return 0u;
}
void ActorControlUpdateRuntimeStub::AddClump(std::uint32_t) {}
void ActorControlUpdateRuntimeStub::RemoveClump(std::uint32_t) {}
void ActorControlUpdateRuntimeStub::ProcessClumpAtomics(std::uint32_t) {}
void ActorControlUpdateRuntimeStub::ConfigureTransition(
    ActorControlDatabasePrefix&, const ActorControlVector&,
    const ActorControlVector&, float) {}
void ActorControlUpdateRuntimeStub::PlayerCommand(
    ActorControlPlayerPrefix&, std::string_view) {}
void ActorControlUpdateRuntimeStub::SnapPlayerPosition(
    ActorControlPlayerPrefix&, const ActorControlVector&,
    std::uint32_t) {}
void ActorControlUpdateRuntimeStub::SetAnimationFlag(
    ActorControlPlayerPrefix&, std::uint32_t) {}
void ActorControlUpdateRuntimeStub::ProcessUiEvents(std::uint32_t) {}
void ActorControlUpdateRuntimeStub::SetMonsterFlags() {}
void ActorControlUpdateRuntimeStub::PlayMusic(std::string_view,
                                               std::uint32_t) {}
ActorControlVector ActorControlUpdateRuntimeStub::ScaleMovement(float) {
    return {};
}
float ActorControlUpdateRuntimeStub::MovementHeight() { return 0.0f; }
double IActorControlUpdateRuntime::PlanarRangeMagnitude(
    const ActorControlVector& value) {
    return GameVectorMath::ApproximateMagnitude(value);
}
float ActorControlUpdateRuntimeStub::VectorLength(
    const ActorControlVector& value) {
    return std::sqrt(value[0] * value[0] + value[1] * value[1] +
                     value[2] * value[2]);
}
void ActorControlUpdateRuntimeStub::ProcessMovement(
    ActorControlPlayerPrefix&, std::uint32_t,
    std::uint32_t, std::uint32_t,
    const ActorControlMovementBounds&, ActorControlVector&) {}
void ActorControlUpdateRuntimeStub::AdjustPlayerPosition(
    ActorControlVector&, float, float, std::uint32_t) {}
void ActorControlUpdateRuntimeStub::SetPlayerPosition(
    ActorControlPlayerPrefix&, const ActorControlVector&) {}
void ActorControlUpdateRuntimeStub::UpdateMinimap(
    const ActorControlPlayerPrefix&) {}
ActorControlVector ActorControlUpdateRuntimeStub::CameraPosition() {
    return {};
}
void ActorControlUpdateRuntimeStub::SetAudioPosition(
    const ActorControlVector&) {}
std::uint32_t ActorControlUpdateRuntimeStub::CollisionObject() { return 0u; }
bool ActorControlUpdateRuntimeStub::SweepCollision(
    const ActorControlVector&, const ActorControlVector&,
    std::uint32_t, ActorControlCollisionResult&) { return false; }
void ActorControlUpdateRuntimeStub::ResolvePlayerCollision(
    ActorControlPlayerPrefix&, const ActorControlVector&,
    std::uint32_t) {}
bool ActorControlUpdateRuntimeStub::NameNodeNeedsUpdate(
    ActorControlDatabasePrefix&) { return false; }
void ActorControlUpdateRuntimeStub::HandleNameEvent(
    ActorControlDatabasePrefix&) {}
void ActorControlUpdateRuntimeStub::HandleNameEventWithContext(
    ActorControlDatabasePrefix&, std::uint32_t, std::uint32_t) {}
std::string_view ActorControlUpdateRuntimeStub::FloorSound() { return {}; }
void ActorControlUpdateRuntimeStub::PlayFloorSound(
    std::string_view, const ActorControlVector&, std::uint32_t,
    float, float) {}
std::array<std::uint32_t, 2> ActorControlUpdateRuntimeStub::CreateEffect(
    std::string_view, const ActorControlVector&, std::uint32_t,
    std::uint32_t) { return {}; }
void ActorControlUpdateRuntimeStub::ReleaseTemporaryEffect() {}
void ActorControlUpdateRuntimeStub::SetEffectVisibility(
    ActorControlDatabasePrefix&, const ActorControlPlayerPrefix&) {}
void ActorControlUpdateRuntimeStub::InitializeScript() {}
std::string_view ActorControlUpdateRuntimeStub::ArenaScriptPath() { return {}; }
bool ActorControlUpdateRuntimeStub::ScriptCached(std::string_view) {
    return false;
}
std::uint32_t ActorControlUpdateRuntimeStub::CreateInterpreter(
    std::string_view, std::string_view, std::string_view) { return 0u; }
bool ActorControlUpdateRuntimeStub::InterpreterRaisedException(
    std::uint32_t) {
    return false;
}
void ActorControlUpdateRuntimeStub::HandleScriptException(std::uint32_t) {}
void ActorControlUpdateRuntimeStub::ReleaseInterpreter(std::uint32_t) {}
void ActorControlUpdateRuntimeStub::ShowScriptError() {}
void ActorControlUpdateRuntimeStub::CleanupScripts() {}

std::uint32_t ActorControlDatabase::Update(
    ActorControlDatabasePrefix& state, ActorControlPlayerPrefix& player,
    float delta_seconds, IActorControlUpdateRuntime& runtime) {
    auto& words = state.words;
    auto forward = runtime.PlayerForward(player);
    static_cast<void>(runtime.PlayerProperty(player));

    if (words[30] == 0u && !runtime.InteractionBlocked(state)) {
        if (runtime.ProgressVisible()) {
            const auto progress = runtime.ProgressValue();
            constexpr double kMidProgress =
                static_cast<double>(std::bit_cast<float>(0x3f4ccccdu));
            if (progress > kMidProgress && progress < 1.0) {
                WriteFloat(words[20], 240.0f);
                WriteFloat(words[21], 20.0f);
            } else if (progress > 0.5 && progress < kMidProgress) {
                WriteFloat(words[20], 160.0f);
                WriteFloat(words[21], 15.0f);
            } else {
                WriteFloat(words[20], 100.0f);
                WriteFloat(words[21], 15.0f);
            }
        }
        const auto input = runtime.ReadInput();
        if (input.held <= 0 || (input.action & 0xfu) == 0u) {
            const auto released = runtime.ReadInput();
            if (released.held == 0 && released.released != 0u &&
                words[29] != 0u) {
                words[29] = 0u;
                words[31] = 0u;
                words[28] = 1u;
                words[30] = 1u;
                const auto start = ReadPosition(player);
                WritePosition(state, 14u, start);
                runtime.Normalize(forward);
                const auto radius = ReadFloat(words[20]);
                const ActorControlVector finish{
                    start[0] + radius * forward[0], start[1],
                    start[2] + radius * forward[2]};
                WritePosition(state, 17u, finish);
                const auto ground = runtime.CheckGround(finish, 100.0f, 100.0f);
                const auto difference = ground.height - finish[1];
                words[34] = !ground.found ||
                            !(difference <= ReadFloat(words[21]));
                runtime.ConfigureTransition(state, start, finish, 588.0f);
                words[13] = 0u;
                runtime.PlayerCommand(player, "C06_1");
                runtime.SetAnimationFlag(player, 0u);
                runtime.SetProgressVisible(false);
                runtime.RemoveClump(words[40]);
                runtime.ProcessUiEvents(0u);
                runtime.SetMonsterFlags();
                runtime.PlayMusic("gamedata\\PALSound\\WF027.wav", 1u);
            }
            return words[30];
        }

        if (!runtime.ProgressVisible()) runtime.SetProgressVisible(true);
        const auto start = ReadPosition(player);
        runtime.Normalize(forward);
        const auto radius = ReadFloat(words[20]);
        ActorControlVector target{start[0] + radius * forward[0],
                                  start[1],
                                  start[2] + radius * forward[2]};
        const auto ground = runtime.FindGround(target, 100.0f, 100.0f);
        if (!ground.found || !(ground.height >
                               std::numeric_limits<float>::lowest())) {
            runtime.RemoveClump(words[40]);
        } else {
            target[1] = ground.height;
            runtime.SetResourcePosition(state, target, 0u);
            runtime.MoveResource(state,
                                 {0.0f, delta_seconds * 200.0f, 0.0f}, 1u);
            if (runtime.HAnimFrameId(words[40]) == 0u) {
                runtime.AddClump(words[40]);
                runtime.ProcessClumpAtomics(words[40]);
            }
        }
        words[29] = 1u;
        return words[30];
    }

    if (words[31] != 0u) return words[30];

    auto position = ReadPosition(player);
    auto movement = runtime.ScaleMovement(delta_seconds);
    const auto property = runtime.PlayerProperty(player);
    ActorControlMovementBounds bounds{};
    for (std::size_t i = 0u; i < 4u; ++i) {
        bounds.upper[i] = ReadFloat(player.words[56u + i]);
        bounds.lower[i] = bounds.upper[i];
    }
    const auto height = runtime.MovementHeight();
    const auto half_property =
        static_cast<float>(runtime.PlayerProperty(player) * 0.5);
    bounds.upper[1] += 0.25f * half_property;
    bounds.lower[1] -= half_property * 0.35f;
    bounds.upper[3] = height;
    bounds.lower[3] = height;
    const auto start = ReadStatePosition(state, 14u);
    //422D77..D9D subtracts startY from itself, not the player's current Y.
    if (runtime.PlanarRangeMagnitude(
            {position[0] - start[0], 0.0f, position[2] - start[2]}) >
        ReadFloat(words[20]))
        words[34] = 1u;

    const auto old_position = position;
    const auto vertical = movement[1];
    runtime.ProcessMovement(player, 1u, words[34], 1u, bounds, movement);
    movement[1] = vertical;
    for (std::size_t i = 0u; i < 3u; ++i) position[i] += movement[i];
    runtime.AdjustPlayerPosition(
        position, static_cast<float>(property * 0.35), 2000.0f, 0u);
    runtime.SetPlayerPosition(player, position);
    runtime.UpdateMinimap(player);
    auto camera = runtime.CameraPosition();
    camera[2] = -camera[2];
    runtime.SetAudioPosition(camera);

    words[13] = words[12];
    if (words[12] == 1u) {
        WritePosition(state, 22u, position);
        return words[30];
    }
    if (words[12] != 2u) return words[30];
    WritePosition(state, 25u, position);

    const auto traveled = runtime.VectorLength(Difference(
        position, ReadStatePosition(state, 22u)));
    if (!(traveled > 600.0f)) {
        const auto object = runtime.CollisionObject();
        ActorControlCollisionResult collision{};
        if (object != 0u &&
            runtime.SweepCollision(old_position, position, object,
                                   collision)) {
            runtime.ResolvePlayerCollision(player, collision.response, 0u);
            runtime.PlayerCommand(player, "C06_2");
            runtime.SetAnimationFlag(player, 0u);
            words[31] = 1u;
            if (runtime.NameNodeNeedsUpdate(state))
                runtime.HandleNameEvent(state);
            const auto sound = runtime.FloorSound();
            if (!sound.empty())
                runtime.PlayFloorSound(sound, old_position, 1u,
                                       600.0f, 300.0f);
        }
        return words[30];
    }

    if (!runtime.NameNodeNeedsUpdate(state)) {
        const auto effect = runtime.CreateEffect("danger", position, 1u, 0u);
        words[128] = effect[0];
        words[129] = effect[1];
        runtime.ReleaseTemporaryEffect();
        runtime.SetEffectVisibility(state, player);
    }
    runtime.InitializeScript();
    const std::string base_path(runtime.ArenaScriptPath());
    const auto path = base_path + "[danger]";
    if (!runtime.ScriptCached(path)) {
        const auto interpreter = runtime.CreateInterpreter(
            base_path, path, "danger");
        if (interpreter != 0u &&
            runtime.InterpreterRaisedException(interpreter)) {
            runtime.HandleScriptException(interpreter);
            runtime.ReleaseInterpreter(interpreter);
        } else {
            runtime.ShowScriptError();
        }
    }
    runtime.CleanupScripts();
    words[31] = 1u;
    return 1u;
}

std::uint32_t ActorControlDatabase::GetDisplayState(
    const ActorControlDatabasePrefix& state) {
    if (state.words[30] != 0u || state.words[33] != 0u) return 0u;
    return state.words[32];
}

void ActorControlDatabase::ResetPlayer(
    ActorControlDatabasePrefix& state, ActorControlPlayerPrefix* player,
    std::uint32_t event_context, IActorControlUpdateRuntime& runtime) {
    if (player != nullptr) {
        runtime.SnapPlayerPosition(*player,
                                   ReadStatePosition(state, 14u), 0u);
        runtime.PlayerCommand(*player, "C01");
        runtime.SetAnimationFlag(*player, 1u);
        if (runtime.NameNodeNeedsUpdate(state))
            runtime.HandleNameEventWithContext(state, event_context, 1u);
    }
    state.words[31] = 1u;
    state.words[28] = 0u;
    state.words[29] = 0u;
    state.words[30] = 0u;
}

}  // namespace pal4
