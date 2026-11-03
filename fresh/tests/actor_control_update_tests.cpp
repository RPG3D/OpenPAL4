#include "pal4/actor_control_update.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

using Vector = pal4::ActorControlVector;
using State = pal4::ActorControlDatabasePrefix;
using Player = pal4::ActorControlPlayerPrefix;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void SetFloat(std::uint32_t& word, float value) {
    word = std::bit_cast<std::uint32_t>(value);
}

float GetFloat(std::uint32_t word) {
    return std::bit_cast<float>(word);
}

void SetPosition(Player& player, Vector value) {
    for (std::size_t i = 0u; i < 3u; ++i)
        SetFloat(player.words[43u + i], value[i]);
}

void SetStatePosition(State& state, std::size_t start, Vector value) {
    for (std::size_t i = 0u; i < 3u; ++i)
        SetFloat(state.words[start + i], value[i]);
}

struct Runtime final : pal4::ActorControlUpdateRuntimeStub {
    Vector PlayerForward(const Player&) override {
        events.emplace_back("forward");
        return forward;
    }
    double PlayerProperty(const Player&) override {
        events.emplace_back("property");
        ++property_calls;
        return property;
    }
    bool InteractionBlocked(State&) override {
        events.emplace_back("gate");
        return blocked;
    }
    bool ProgressVisible() override {
        events.emplace_back("visible?");
        return progress_visible;
    }
    double ProgressValue() override {
        events.emplace_back("progress");
        return progress;
    }
    void SetProgressVisible(bool value) override {
        events.emplace_back("set visible");
        progress_visible = value;
    }
    pal4::ActorControlInput ReadInput() override {
        events.emplace_back("input");
        return input_index < inputs.size() ? inputs[input_index++]
                                           : pal4::ActorControlInput{};
    }
    void Normalize(Vector& value) override {
        events.emplace_back("normalize");
        pal4::ActorControlUpdateRuntimeStub::Normalize(value);
    }
    pal4::ActorControlGroundResult FindGround(
        const Vector& value, float radius, float height) override {
        events.emplace_back("find ground");
        ground_target = value;
        Check(radius == 100.0f && height == 100.0f,
              "ground lookup uses hundred-unit parameters");
        return ground;
    }
    pal4::ActorControlGroundResult CheckGround(
        const Vector& value, float radius, float height) override {
        events.emplace_back("check ground");
        ground_target = value;
        Check(radius == 100.0f && height == 100.0f,
              "release ground check uses hundred-unit parameters");
        return ground;
    }
    void SetResourcePosition(State&, const Vector& value,
                             std::uint32_t mode) override {
        events.emplace_back("resource position");
        Check(mode == 0u, "resource position forwards mode zero");
        resource_position = value;
    }
    void MoveResource(State&, const Vector& value,
                      std::uint32_t mode) override {
        events.emplace_back("resource move");
        Check(mode == 1u, "resource movement forwards mode one");
        resource_velocity = value;
    }
    std::uint32_t HAnimFrameId(std::uint32_t clump) override {
        events.emplace_back("frame id");
        checked_clump = clump;
        return frame_id;
    }
    void AddClump(std::uint32_t clump) override {
        events.emplace_back("add clump");
        checked_clump = clump;
    }
    void RemoveClump(std::uint32_t clump) override {
        events.emplace_back("remove clump");
        checked_clump = clump;
    }
    void ProcessClumpAtomics(std::uint32_t clump) override {
        events.emplace_back("atomics");
        checked_clump = clump;
    }
    void ConfigureTransition(State&, const Vector& start,
                             const Vector& finish, float range) override {
        events.emplace_back("transition");
        Check(range == 588.0f,
              "transition forwards observed range constant");
        transition_start = start;
        transition_finish = finish;
    }
    void PlayerCommand(Player&, std::string_view command) override {
        events.emplace_back("command");
        last_command = command;
    }
    void SnapPlayerPosition(Player&, const Vector& position,
                            std::uint32_t mode) override {
        events.emplace_back("snap player");
        Check(mode == 0u, "reset snaps player with mode zero");
        snapped_position = position;
    }
    void SetAnimationFlag(Player&, std::uint32_t flag) override {
        events.emplace_back("animation");
        Check(flag == (last_command == "C01" ? 1u : 0u),
              "reset enables and interaction clears animation flag");
    }
    void ProcessUiEvents(std::uint32_t mode) override {
        events.emplace_back("ui events");
        Check(mode == 0u, "interaction processes zero-mode UI events");
    }
    void SetMonsterFlags() override {
        events.emplace_back("monster flags");
    }
    void PlayMusic(std::string_view path, std::uint32_t mode) override {
        events.emplace_back("music");
        Check(path == "gamedata\\PALSound\\WF027.wav" && mode == 1u,
              "release plays observed WF027 path");
    }
    Vector ScaleMovement(float seconds) override {
        events.emplace_back("scale movement");
        scale_seconds = seconds;
        return movement;
    }
    float MovementHeight() override {
        events.emplace_back("movement height");
        return movement_height;
    }
    void ProcessMovement(Player&, std::uint32_t first,
                         std::uint32_t mode, std::uint32_t third,
                         const pal4::ActorControlMovementBounds& value,
                         Vector& delta) override {
        events.emplace_back("process movement");
        Check(first == 1u && third == 1u,
              "movement forwards both fixed mode flags");
        collision_mode = mode;
        bounds = value;
        delta[0] += 1.0f;
        delta[1] = 999.0f;
    }
    void AdjustPlayerPosition(Vector& value, float radius,
                              float range, std::uint32_t mode) override {
        events.emplace_back("adjust position");
        Check(range == 2000.0f && mode == 0u,
              "position correction forwards range and mode");
        adjusted_radius = radius;
        adjusted_position = value;
    }
    void SetPlayerPosition(Player&, const Vector& value) override {
        events.emplace_back("set player");
        player_position = value;
    }
    void UpdateMinimap(const Player&) override {
        events.emplace_back("minimap");
    }
    Vector CameraPosition() override {
        events.emplace_back("camera");
        return {1.0f, 2.0f, 3.0f};
    }
    void SetAudioPosition(const Vector& value) override {
        events.emplace_back("audio position");
        audio_position = value;
    }
    std::uint32_t CollisionObject() override {
        events.emplace_back("collision object");
        return collision_object;
    }
    bool SweepCollision(const Vector& start, const Vector& finish,
                        std::uint32_t object,
                        pal4::ActorControlCollisionResult& result) override {
        events.emplace_back("sweep");
        Check(object == collision_object,
              "sweep receives collision object");
        sweep_start = start;
        sweep_finish = finish;
        result.contact = {1.0f, 2.0f, 3.0f};
        result.response = {4.0f, 5.0f, 6.0f};
        return collision_hit;
    }
    void ResolvePlayerCollision(Player&, const Vector& hit,
                                std::uint32_t mode) override {
        events.emplace_back("resolve collision");
        Check(hit == Vector{4.0f, 5.0f, 6.0f} && mode == 0u,
              "collision resolution receives sweep result");
    }
    bool NameNodeNeedsUpdate(State&) override {
        events.emplace_back("name?");
        return name_updated;
    }
    void HandleNameEvent(State&) override {
        events.emplace_back("name event");
    }
    void HandleNameEventWithContext(State&, std::uint32_t context,
                                    std::uint32_t mode) override {
        events.emplace_back("name event context");
        event_context = context;
        Check(mode == 1u, "reset forwards name-event mode one");
    }
    std::string_view FloorSound() override {
        events.emplace_back("floor sound");
        return floor_sound;
    }
    void PlayFloorSound(std::string_view sound,
                        const Vector& position, std::uint32_t mode,
                        float radius, float volume) override {
        events.emplace_back("play floor sound");
        Check(sound == floor_sound && mode == 1u &&
                  radius == 600.0f && volume == 300.0f,
              "floor audio uses selected sound and 3D parameters");
        floor_sound_position = position;
    }
    std::array<std::uint32_t, 2> CreateEffect(
        std::string_view name, const Vector& position,
        std::uint32_t first, std::uint32_t second) override {
        events.emplace_back("create effect");
        Check(name == "danger" && first == 1u && second == 0u,
              "effect creation forwards observed name and flags");
        effect_position = position;
        return {0x1111u, 0x2222u};
    }
    void ReleaseTemporaryEffect() override {
        events.emplace_back("release effect");
    }
    void SetEffectVisibility(State&, const Player&) override {
        events.emplace_back("effect visibility");
    }
    void InitializeScript() override {
        events.emplace_back("script init");
    }
    std::string_view ArenaScriptPath() override {
        events.emplace_back("script base");
        return "scripts/";
    }
    bool ScriptCached(std::string_view path) override {
        events.emplace_back("cache");
        Check(path == "scripts/[danger]", "danger path joins arena base");
        return cached;
    }
    std::uint32_t CreateInterpreter(std::string_view base,
                                    std::string_view path,
                                    std::string_view name) override {
        events.emplace_back("interpreter");
        Check(base == "scripts/" && path == "scripts/[danger]" &&
                  name == "danger",
              "interpreter receives observed script names");
        return interpreter;
    }
    bool InterpreterRaisedException(std::uint32_t value) override {
        events.emplace_back("interpreter exception?");
        Check(value == interpreter,
              "exception predicate receives created interpreter");
        return exception_raised;
    }
    void HandleScriptException(std::uint32_t value) override {
        events.emplace_back("script exception");
        Check(value == interpreter, "exception receives interpreter");
    }
    void ReleaseInterpreter(std::uint32_t value) override {
        events.emplace_back("release interpreter");
        Check(value == interpreter, "release receives interpreter");
    }
    void ShowScriptError() override {
        events.emplace_back("script error");
    }
    void CleanupScripts() override {
        events.emplace_back("script cleanup");
    }

    Vector forward{1.0f, 0.0f, 0.0f};
    double property{20.0};
    std::uint32_t property_calls{};
    bool blocked{};
    bool progress_visible{};
    double progress{};
    std::vector<pal4::ActorControlInput> inputs;
    std::size_t input_index{};
    pal4::ActorControlGroundResult ground{};
    Vector ground_target{};
    Vector resource_position{};
    Vector resource_velocity{};
    Vector transition_start{};
    Vector transition_finish{};
    std::uint32_t frame_id{};
    std::uint32_t checked_clump{};
    std::string_view last_command{};
    Vector snapped_position{};
    std::uint32_t event_context{};
    Vector movement{2.0f, 3.0f, 4.0f};
    float scale_seconds{};
    float movement_height{7.0f};
    std::uint32_t collision_mode{};
    pal4::ActorControlMovementBounds bounds{};
    float adjusted_radius{};
    Vector adjusted_position{};
    Vector player_position{};
    Vector audio_position{};
    std::uint32_t collision_object{};
    bool collision_hit{};
    Vector sweep_start{};
    Vector sweep_finish{};
    bool name_updated{};
    std::string_view floor_sound{};
    Vector floor_sound_position{};
    Vector effect_position{};
    bool cached{};
    std::uint32_t interpreter{};
    bool exception_raised{};
    std::vector<std::string> events;
};

void TestIdleAndProgressThresholds() {
    for (const auto [progress, distance, limit] :
         {std::array<double, 3>{0.5, 100.0, 15.0},
          {0.6, 160.0, 15.0},
          {static_cast<double>(std::bit_cast<float>(0x3f4ccccdu)),
           100.0, 15.0},
          {0.9, 240.0, 20.0},
          {1.0, 100.0, 15.0}}) {
        Runtime runtime;
        State state{};
        Player player{};
        runtime.progress_visible = true;
        runtime.progress = progress;
        Check(pal4::ActorControlDatabase::Update(
                  state, player, 0.1f, runtime) == 0u &&
                  GetFloat(state.words[20]) == distance &&
                  GetFloat(state.words[21]) == limit &&
                  runtime.property_calls == 1u &&
                  runtime.input_index == 0u &&
                  runtime.events == std::vector<std::string>{
                      "forward", "property", "gate", "visible?",
                      "progress", "input", "input"},
              "idle progress thresholds and two input reads");
    }
    Runtime blocked;
    State state{};
    Player player{};
    blocked.blocked = true;
    state.words[31] = 1u;
    Check(pal4::ActorControlDatabase::Update(
              state, player, 0.1f, blocked) == 0u &&
              blocked.events == std::vector<std::string>{
                  "forward", "property", "gate"},
          "blocked interaction with completed state returns early");
}

void TestHeldInput() {
    Runtime runtime;
    State state{};
    Player player{};
    SetPosition(player, {10.0f, 20.0f, 30.0f});
    SetFloat(state.words[20], 100.0f);
    state.words[40] = 0x9000u;
    runtime.inputs.push_back({1, 1u, 0u});
    runtime.ground = {true, 5.0f};
    Check(pal4::ActorControlDatabase::Update(
              state, player, 0.25f, runtime) == 0u &&
              state.words[29] == 1u && runtime.progress_visible &&
              runtime.ground_target == Vector{110.0f, 20.0f, 30.0f} &&
              runtime.resource_position == Vector{110.0f, 5.0f, 30.0f} &&
              runtime.resource_velocity == Vector{0.0f, 50.0f, 0.0f} &&
              runtime.checked_clump == 0x9000u,
          "held input moves and publishes a new clump");
    Check(runtime.events == std::vector<std::string>{
              "forward", "property", "gate", "visible?", "input",
              "visible?", "set visible", "normalize", "find ground",
              "resource position", "resource move", "frame id",
              "add clump", "atomics"},
          "held-input engine calls retain observed order");

    Runtime existing;
    State already_loaded{};
    already_loaded.words[40] = 0x9000u;
    SetFloat(already_loaded.words[20], 100.0f);
    existing.inputs.push_back({1, 1u, 0u});
    existing.ground = {true, 5.0f};
    existing.frame_id = 1u;
    Check(pal4::ActorControlDatabase::Update(
              already_loaded, player, 0.1f, existing) == 0u &&
              existing.events.back() == "frame id" &&
              already_loaded.words[29] == 1u,
          "an existing HAnim frame skips arena insertion");

    Runtime invalid;
    State failed{};
    failed.words[40] = 0x9000u;
    SetFloat(failed.words[20], 100.0f);
    invalid.inputs.push_back({1, 1u, 0u});
    invalid.ground = {false, 0.0f};
    Check(pal4::ActorControlDatabase::Update(
              failed, player, 0.1f, invalid) == 0u &&
              failed.words[29] == 1u &&
              invalid.events.back() == "remove clump",
          "invalid ground removes clump but records held state");
}

void TestRelease() {
    Runtime runtime;
    State state{};
    Player player{};
    SetPosition(player, {10.0f, 20.0f, 30.0f});
    SetFloat(state.words[20], 100.0f);
    SetFloat(state.words[21], 15.0f);
    state.words[29] = 1u;
    state.words[40] = 0x9000u;
    runtime.inputs = {{0, 0u, 0u}, {0, 0u, 1u}};
    runtime.ground = {true, 50.0f};
    Check(pal4::ActorControlDatabase::Update(
              state, player, 0.1f, runtime) == 1u &&
              state.words[29] == 0u && state.words[31] == 0u &&
              state.words[28] == 1u && state.words[30] == 1u &&
              state.words[34] == 1u && state.words[13] == 0u &&
              runtime.transition_start == Vector{10.0f, 20.0f, 30.0f} &&
              runtime.transition_finish == Vector{110.0f, 20.0f, 30.0f} &&
              runtime.last_command == "C06_1",
          "release initializes the transition and collision mode");
    Check(runtime.events == std::vector<std::string>{
              "forward", "property", "gate", "visible?", "input",
              "input", "normalize", "check ground", "transition",
              "command", "animation", "set visible", "remove clump",
              "ui events", "monster flags", "music"},
          "release side effects follow the disassembly order");
}

void TestMovementAndCollision() {
    Runtime runtime;
    State state{};
    Player player{};
    state.words[30] = 1u;
    state.words[12] = 1u;
    SetFloat(state.words[20], 100.0f);
    SetPosition(player, {10.0f, 20.0f, 30.0f});
    SetStatePosition(state, 14u, {0.0f, 20.0f, 30.0f});
    for (std::size_t i = 0u; i < 4u; ++i)
        SetFloat(player.words[56u + i], static_cast<float>(i + 1u));
    Check(pal4::ActorControlDatabase::Update(
              state, player, 0.2f, runtime) == 1u &&
              state.words[13] == 1u &&
              runtime.player_position == Vector{13.0f, 23.0f, 34.0f} &&
              runtime.property_calls == 3u &&
              runtime.adjusted_position == runtime.player_position &&
              runtime.audio_position == Vector{1.0f, 2.0f, -3.0f} &&
              runtime.adjusted_radius == 7.0f &&
              runtime.bounds.upper == std::array<float, 4>{1.0f, 4.5f,
                                                           3.0f, 7.0f} &&
              runtime.bounds.lower == std::array<float, 4>{1.0f, -1.5f,
                                                           3.0f, 7.0f} &&
              GetFloat(state.words[22]) == 13.0f &&
              GetFloat(state.words[24]) == 34.0f,
          "movement preserves vertical delta and records mode-one position");

    Runtime collision;
    State near{};
    near.words[30] = 1u;
    near.words[12] = 2u;
    SetFloat(near.words[20], 100.0f);
    SetStatePosition(near, 22u, {0.0f, 0.0f, 0.0f});
    collision.collision_object = 0x1234u;
    collision.collision_hit = true;
    collision.name_updated = true;
    collision.floor_sound = "step.wav";
    Check(pal4::ActorControlDatabase::Update(
              near, player, 0.2f, collision) == 1u &&
              near.words[13] == 2u && near.words[31] == 1u &&
              collision.last_command == "C06_2" &&
              collision.floor_sound_position == Vector{10.0f, 20.0f,
                                                       30.0f} &&
              collision.sweep_start == Vector{10.0f, 20.0f, 30.0f} &&
              collision.sweep_finish == Vector{13.0f, 23.0f, 34.0f},
          "near collision stops motion, dispatches event and plays sound");
    Check(collision.events.end()[-7] == "resolve collision" &&
              collision.events.back() == "play floor sound",
          "collision cleanup follows sweep result");

    Runtime boundary;
    State at_limit{};
    Player distant{};
    at_limit.words[30] = 1u;
    at_limit.words[12] = 2u;
    SetPosition(distant, {700.0f, 0.0f, 0.0f});
    SetStatePosition(at_limit, 22u, {103.0f, 3.0f, 4.0f});
    boundary.collision_object = 0u;
    Check(pal4::ActorControlDatabase::Update(
              at_limit, distant, 0.1f, boundary) == 1u &&
              boundary.events.back() == "collision object" &&
              at_limit.words[31] == 0u,
          "exactly 600 units stays in the collision path");
}

void TestHorizontalRangeIgnoresJumpHeight() {
    for (const float height : {0.0f, 500.0f}) {
        Runtime runtime;
        State state{};
        Player player{};
        state.words[30] = 1u;
        state.words[12] = 1u;
        SetFloat(state.words[20], 100.0f);
        SetPosition(player, {30.0f, height, 40.0f});
        SetStatePosition(state, 14u, {0.0f, 0.0f, 0.0f});
        (void)pal4::ActorControlDatabase::Update(state, player, 0.017f, runtime);
        Check(state.words[34] == 0u && runtime.collision_mode == 0u,
              "jump height does not make a fifty-unit horizontal trip exceed range");
    }
    Runtime outside;
    State state{};
    Player player{};
    state.words[30] = 1u;
    state.words[12] = 1u;
    SetFloat(state.words[20], 100.0f);
    SetPosition(player, {300.0f, 500.0f, 400.0f});
    (void)pal4::ActorControlDatabase::Update(state, player, 0.017f, outside);
    Check(state.words[34] == 1u && outside.collision_mode == 1u,
          "horizontal travel beyond the configured range changes collision mode");
}

void TestDangerScript() {
    for (int scenario = 0; scenario < 4; ++scenario) {
        Runtime runtime;
        State state{};
        Player player{};
        state.words[30] = 1u;
        state.words[12] = 2u;
        SetPosition(player, {700.0f, 0.0f, 0.0f});
        SetStatePosition(state, 22u, {0.0f, 0.0f, 0.0f});
        runtime.cached = scenario == 0;
        runtime.interpreter = (scenario == 1 || scenario == 3)
                                  ? 0x1234u : 0u;
        runtime.exception_raised = scenario == 1;
        Check(pal4::ActorControlDatabase::Update(
                  state, player, 0.1f, runtime) == 1u &&
                  state.words[31] == 1u &&
                  state.words[128] == 0x1111u &&
                  state.words[129] == 0x2222u &&
                  runtime.events.back() == "script cleanup",
              "danger path creates an effect and marks completion");
        const auto has = [&](const char* event) {
            for (const auto& recorded : runtime.events)
                if (recorded == event) return true;
            return false;
        };
        Check(has("create effect") && has("release effect") &&
                  has("effect visibility") && has("script init") &&
                  has("cache") &&
                  has("script exception") == (scenario == 1) &&
                  has("release interpreter") == (scenario == 1) &&
                  has("script error") ==
                      (scenario == 2 || scenario == 3),
              "cached, successful, missing and failed scripts diverge");
    }
}

void TestResetPlayer() {
    Runtime runtime;
    State state{};
    Player player{};
    state.words[28] = state.words[29] = state.words[30] = 1u;
    state.words[31] = 0u;
    SetStatePosition(state, 14u, {10.0f, 20.0f, 30.0f});
    runtime.name_updated = true;
    pal4::ActorControlDatabase::ResetPlayer(
        state, &player, 0x1234u, runtime);
    Check(state.words[28] == 0u && state.words[29] == 0u &&
              state.words[30] == 0u && state.words[31] == 1u &&
              runtime.snapped_position == Vector{10.0f, 20.0f,
                                                 30.0f} &&
              runtime.last_command == "C01" &&
              runtime.event_context == 0x1234u &&
              runtime.events == std::vector<std::string>{
                  "snap player", "command", "animation", "name?",
                  "name event context"},
          "player reset restores position and dispatches name event");

    Runtime no_player;
    State absent{};
    absent.words[28] = absent.words[29] = absent.words[30] = 9u;
    pal4::ActorControlDatabase::ResetPlayer(
        absent, nullptr, 0x55u, no_player);
    Check(absent.words[28] == 0u && absent.words[29] == 0u &&
              absent.words[30] == 0u && absent.words[31] == 1u &&
              no_player.events.empty(),
          "null player skips engine calls but resets state");
}

}  // namespace

int main() {
    TestIdleAndProgressThresholds();
    TestHeldInput();
    TestRelease();
    TestMovementAndCollision();
    TestHorizontalRangeIgnoresJumpHeight();
    TestDangerScript();
    TestResetPlayer();
}
