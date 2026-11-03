#include "pal4/actor_control_motion.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ActorControlMotionRuntimeStub {
    std::uint32_t FreeAddress(
        pal4::ActorControlMotionState* state) override {
        freed.push_back(state);
        return 0x7777u;
    }
    std::vector<pal4::ActorControlMotionState*> freed;
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words[address];
    }
    pal4::MotionVector ConvertPosition(
        const std::array<std::uint32_t, 3>& raw) override {
        events.emplace_back("convert");
        return pal4::ActorControlMotionRuntimeStub::ConvertPosition(raw);
    }
    bool PositionsDiffer(const pal4::MotionVector& left,
                         const pal4::MotionVector& right) override {
        events.emplace_back("different");
        return left != right;
    }
    std::uint32_t GameArenaManager() override {
        events.emplace_back("arena");
        return 0x9000u;
    }
    std::uint32_t CollisionProbe(std::uint32_t arena,
                                  const pal4::MotionVector& target) override {
        events.emplace_back("probe");
        probed_arena = arena;
        probed_target = target;
        return probe_result;
    }
    bool TryMove(std::uint32_t collision,
                 const pal4::MotionVector& current,
                 const pal4::MotionVector& target,
                 std::uint32_t probe, std::uint32_t mode) override {
        events.emplace_back("try move");
        move_args = {collision, probe, mode};
        move_current = current;
        move_target = target;
        return move_result;
    }
    void SetMovementSpeed(std::uint32_t collision,
                          std::uint32_t bits) override {
        events.emplace_back("speed");
        speed_args = {collision, bits};
    }
    void SetMovementTarget(std::uint32_t collision,
                           const pal4::MotionVector& target) override {
        events.emplace_back("target");
        target_collision = collision;
        movement_target = target;
    }
    void ActorCommand(std::uint32_t controller,
                      const char* command) override {
        events.emplace_back("command");
        command_controller = controller;
        commands.emplace_back(command);
    }
    bool CollisionBlocked(std::uint32_t collision) override {
        events.emplace_back("blocked");
        blocked_collision = collision;
        return blocked;
    }
    void SnapActorPosition(std::uint32_t actor,
                           const pal4::MotionVector& position,
                           std::uint32_t mode) override {
        events.emplace_back("snap");
        snapped_actor = actor;
        snapped_position = position;
        snapped_mode = mode;
    }
    float VectorMagnitude(const pal4::MotionVector& delta) override {
        events.emplace_back("magnitude");
        measured_delta = delta;
        return magnitude_result;
    }
    std::uint32_t InitializeActorControl() override {
        events.emplace_back("actor control");
        return 0xa000u;
    }
    void ResetActorControl(std::uint32_t control) override {
        events.emplace_back("reset control");
        reset_control = control;
    }
    std::uint32_t UiFrameManager() override {
        events.emplace_back("ui manager");
        return 0xb000u;
    }
    void ProcessUiEvents(std::uint32_t manager,
                         std::uint32_t mode) override {
        events.emplace_back("ui events");
        ui_args = {manager, mode};
    }
    std::uint32_t MonsterManager() override {
        events.emplace_back("monster manager");
        return 0xc000u;
    }
    void ClearMonsterFlags(std::uint32_t manager) override {
        events.emplace_back("clear monsters");
        cleared_monster_manager = manager;
    }
    void UpdateActorPosition(std::uint32_t actor,
                             const pal4::MotionVector& position) override {
        events.emplace_back("update position");
        updated_actor = actor;
        updated_position = position;
    }
    void NormalizeDirection(pal4::MotionVector& direction) override {
        events.emplace_back("normalize");
        const auto length = std::sqrt(direction[0] * direction[0] +
                                      direction[1] * direction[1] +
                                      direction[2] * direction[2]);
        if (length != 0.0f)
            for (auto& component : direction) component /= length;
    }
    std::uint32_t SetMonsterFlags(std::uint32_t manager) override {
        events.emplace_back("set monsters");
        set_monster_manager = manager;
        return 0x7777u;
    }
    void PutVector(std::uint32_t address,
                   const pal4::MotionVector& value) {
        for (std::uint32_t i = 0u; i < 3u; ++i)
            words[address + 4u * i] = std::bit_cast<std::uint32_t>(value[i]);
    }
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::vector<std::string> commands;
    std::uint32_t probe_result{};
    bool move_result{};
    bool blocked{};
    float magnitude_result{};
    std::uint32_t probed_arena{};
    pal4::MotionVector probed_target{};
    std::vector<std::uint32_t> move_args;
    pal4::MotionVector move_current{};
    pal4::MotionVector move_target{};
    std::vector<std::uint32_t> speed_args;
    std::uint32_t target_collision{};
    pal4::MotionVector movement_target{};
    std::uint32_t command_controller{};
    std::uint32_t blocked_collision{};
    std::uint32_t snapped_actor{};
    pal4::MotionVector snapped_position{};
    std::uint32_t snapped_mode{};
    pal4::MotionVector measured_delta{};
    std::uint32_t reset_control{};
    std::vector<std::uint32_t> ui_args;
    std::uint32_t cleared_monster_manager{};
    std::uint32_t updated_actor{};
    pal4::MotionVector updated_position{};
    std::uint32_t set_monster_manager{};
};

std::array<std::uint32_t, 3> Bits(const pal4::MotionVector& value) {
    return {std::bit_cast<std::uint32_t>(value[0]),
            std::bit_cast<std::uint32_t>(value[1]),
            std::bit_cast<std::uint32_t>(value[2])};
}

void TestConstructionAndReset() {
    pal4::ActorControlMotionState state;
    std::memset(&state, 0xa5, sizeof(state));
    Check(&pal4::ActorControlMotion::Construct(state) == &state &&
              state.vtable == pal4::ActorControlMotion::kVtable &&
              state.active == 0u && state.reserved08 == 0u &&
              state.mode == 0u && state.pending == 0u &&
              state.reached == 1u &&
              state.speed_bits ==
                  pal4::ActorControlMotion::kInitialSpeedBits &&
              state.target_position[0] == 0xa5a5a5a5u &&
              state.final_position[2] == 0xa5a5a5a5u &&
              state.direction[1] == 0xa5a5a5a5u &&
              state.threshold_bits == 0xa5a5a5a5u &&
              state.alternate_height_bits == 0xa5a5a5a5u,
          "motion constructor writes only observed vtable and fields");
    state.active = 9u;
    state.reserved08 = 8u;
    state.mode = 7u;
    state.pending = 6u;
    state.reached = 0u;
    state.speed_bits = 0x12345678u;
    Check(pal4::ActorControlMotion::Reset(state) == 1u &&
              state.active == 0u && state.reserved08 == 0u &&
              state.mode == 0u && state.pending == 0u &&
              state.reached == 1u &&
              state.speed_bits == 0x12345678u,
          "first reset preserves speed and returns one");
    state.active = 3u;
    state.pending = 2u;
    Check(pal4::ActorControlMotion::ResetWithArgument(
              state, 0xdeadbeefu) == 1u &&
              state.active == 0u && state.pending == 0u,
          "second reset ignores its stack argument");
}

void TestDeletingWrapper() {
    pal4::ActorControlMotionState state{};
    Runtime runtime;
    state.vtable = pal4::ActorControlMotion::kVtable;
    Check(pal4::ActorControlMotion::DeleteBase(&state, 2u, runtime) ==
              &state &&
              state.vtable == pal4::ActorControlMotion::kBaseVtable &&
              runtime.freed.empty(),
          "base deleting wrapper retains storage without bit zero");
    Check(pal4::ActorControlMotion::DeleteBase(&state, 3u, runtime) ==
              &state &&
              runtime.freed ==
                  std::vector<pal4::ActorControlMotionState*>{&state},
          "base deleting wrapper frees storage when bit zero is set");
}

void TestCollisionProbePath() {
    Runtime runtime;
    pal4::ActorControlMotionState state{};
    constexpr std::uint32_t actor = 0x1000u;
    state.active = 1u;
    state.mode = 1u;
    state.pending = 1u;
    state.target_position = Bits({4.0f, 5.0f, 6.0f});
    runtime.PutVector(actor + 172u, {4.0f, 5.0f, 6.0f});
    Check(pal4::ActorControlMotion::Update(state, actor, 0.1f,
                                             runtime) == 1u &&
              state.pending == 0u &&
              runtime.events == std::vector<std::string>{
                  "convert", "convert", "different"},
          "equal positions clear pending without collision calls");

    state.pending = 1u;
    runtime.PutVector(actor + 172u, {1.0f, 2.0f, 3.0f});
    runtime.words[0x9000u + 404u] = 0x9100u;
    runtime.events.clear();
    Check(pal4::ActorControlMotion::Update(state, actor, 0.1f,
                                             runtime) == 1u &&
              state.pending == 0u &&
              runtime.probed_arena == 0x9100u &&
              runtime.probed_target == pal4::MotionVector{4.0f, 5.0f, 6.0f} &&
              runtime.events.back() == "probe",
          "null collision probe skips movement attempt");

    state.pending = 1u;
    runtime.probe_result = 0x9200u;
    runtime.move_result = false;
    runtime.events.clear();
    Check(pal4::ActorControlMotion::Update(state, actor, 0.1f,
                                             runtime) == 1u &&
              runtime.move_args == std::vector<std::uint32_t>{
                  actor + 1380u, 0x9200u, 0u} &&
              runtime.move_current == pal4::MotionVector{1.0f, 2.0f, 3.0f} &&
              runtime.events.back() == "try move",
          "failed collision attempt leaves controller commands untouched");

    state.pending = 1u;
    runtime.move_result = true;
    runtime.events.clear();
    Check(pal4::ActorControlMotion::Update(state, actor, 0.1f,
                                             runtime) == 1u &&
              runtime.speed_args == std::vector<std::uint32_t>{
                  actor + 1380u, 0x42340000u} &&
              runtime.target_collision == actor + 1380u &&
              runtime.movement_target ==
                  pal4::MotionVector{4.0f, 5.0f, 6.0f} &&
              runtime.command_controller == actor + 368u &&
              runtime.commands.back() == "C02" &&
              runtime.events == std::vector<std::string>{
                  "convert", "convert", "different", "arena", "probe",
                  "try move", "speed", "target", "command"},
          "successful probe sets movement speed, target and C02 command");
}

void TestCollisionCompletionPath() {
    Runtime runtime;
    pal4::ActorControlMotionState state{};
    constexpr std::uint32_t actor = 0x1000u;
    state.active = 1u;
    state.mode = 1u;
    state.reached = 1u;
    runtime.blocked = true;
    Check(pal4::ActorControlMotion::Update(state, actor, 0.0f,
                                             runtime) == 1u &&
              state.mode == 1u &&
              runtime.blocked_collision == actor + 1380u &&
              runtime.events == std::vector<std::string>{"blocked"},
          "blocked collision keeps mode active");
    runtime.blocked = false;
    runtime.PutVector(actor + 160u, {2.0f, 3.0f, 4.0f});
    state.alternate_height_bits = std::bit_cast<std::uint32_t>(7.0f);
    runtime.events.clear();
    Check(pal4::ActorControlMotion::Update(state, actor, 0.0f,
                                             runtime) == 1u &&
              state.mode == 0u &&
              runtime.snapped_actor == actor &&
              runtime.snapped_position ==
                  pal4::MotionVector{2.0f, 7.0f, 4.0f} &&
              runtime.snapped_mode == 0u &&
              runtime.commands.back() == "C04" &&
              runtime.events == std::vector<std::string>{
                  "blocked", "snap", "command"},
          "unblocked collision snaps position and selects C04");
    state.mode = 1u;
    state.reached = 0u;
    runtime.commands.clear();
    Check(pal4::ActorControlMotion::Update(state, actor, 0.0f,
                                             runtime) == 1u &&
              runtime.commands.back() == "C05",
          "unreached state selects C05");
}

void TestTimedMovementAndArrival() {
    Runtime runtime;
    pal4::ActorControlMotionState state{};
    constexpr std::uint32_t actor = 0x1000u;
    Check(pal4::ActorControlMotion::Update(state, actor, 1.0f,
                                             runtime) == 0u &&
              runtime.events.empty(),
          "inactive motion does no work");
    state.active = 1u;
    state.direction = Bits({1.0f, 0.0f, 0.0f});
    state.target_position = Bits({0.0f, 0.0f, 0.0f});
    state.final_position = Bits({9.0f, 8.0f, 7.0f});
    state.speed_bits = std::bit_cast<std::uint32_t>(2.0f);
    state.threshold_bits = std::bit_cast<std::uint32_t>(5.0f);
    runtime.PutVector(actor + 172u, {1.0f, 0.0f, 0.0f});
    runtime.magnitude_result = 4.0f;
    Check(pal4::ActorControlMotion::Update(state, actor, 1.0f,
                                             runtime) == 1u &&
              runtime.measured_delta ==
                  pal4::MotionVector{3.0f, 0.0f, 0.0f} &&
              runtime.updated_position ==
                  pal4::MotionVector{3.0f, 0.0f, 0.0f} &&
              runtime.events == std::vector<std::string>{
                  "magnitude", "update position"},
          "subthreshold movement advances actor without notifications");

    runtime.magnitude_result = 5.0f;
    runtime.events.clear();
    Check(pal4::ActorControlMotion::Update(state, actor, 1.0f,
                                             runtime) == 0u &&
              state.active == 0u && state.mode == 0u &&
              state.pending == 0u && state.reached == 1u &&
              runtime.updated_position ==
                  pal4::MotionVector{9.0f, 8.0f, 7.0f} &&
              runtime.reset_control == 0xa000u &&
              runtime.ui_args ==
                  std::vector<std::uint32_t>{0xb000u, 1u} &&
              runtime.cleared_monster_manager == 0xc000u &&
              runtime.events == std::vector<std::string>{
                  "magnitude", "actor control", "reset control",
                  "ui manager", "ui events", "monster manager",
                  "clear monsters", "update position"},
          "arrival clears state and notifies systems before final position");
}

void TestBeginMotion() {
    Runtime runtime;
    pal4::ActorControlMotionState state{};
    state.speed_bits = std::bit_cast<std::uint32_t>(40.0f);
    runtime.magnitude_result = 5.0f;
    const auto start = Bits({0.0f, 0.0f, 0.0f});
    const auto finish = Bits({3.0f, 4.0f, 0.0f});
    const auto alternate_height = std::bit_cast<std::uint32_t>(7.0f);
    Check(pal4::ActorControlMotion::Begin(
              state, start, finish, alternate_height, 0u,
              runtime) == 0x7777u &&
              state.target_position == start &&
              state.final_position == finish &&
              state.direction == Bits({0.6f, 0.8f, 0.0f}) &&
              state.threshold_bits == std::bit_cast<std::uint32_t>(5.0f) &&
              state.speed_bits == std::bit_cast<std::uint32_t>(40.0f) &&
              state.alternate_height_bits == alternate_height &&
              state.active == 1u && state.mode == 1u &&
              state.pending == 1u && state.reached == 0u &&
              runtime.measured_delta ==
                  pal4::MotionVector{3.0f, 4.0f, 0.0f} &&
              runtime.ui_args ==
                  std::vector<std::uint32_t>{0xb000u, 0u} &&
              runtime.set_monster_manager == 0xc000u &&
              runtime.events == std::vector<std::string>{
                  "magnitude", "normalize", "ui manager", "ui events",
                  "monster manager", "set monsters"},
          "motion begin stores normalized displacement and notifies systems");
}

}  // namespace

int main() {
    TestConstructionAndReset();
    TestDeletingWrapper();
    TestCollisionProbePath();
    TestCollisionCompletionPath();
    TestTimedMovementAndArrival();
    TestBeginMotion();
}
