#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_combat_interaction_callback.h"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

using State = pal4::MonsterBehaviorManagerState;

void SetFloat(State& state, std::uint32_t offset, float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

State InitialState() {
    State state{};
    state.words[0x6e4u / 4u] = 0x7000u;
    state.words[0x654u / 4u] = 0x8000u;
    state.words[0x718u / 4u] = 0xa000u;
    SetFloat(state, 0xacu, 1.0f);
    SetFloat(state, 0xb0u, 2.0f);
    SetFloat(state, 0xb4u, 3.0f);
    SetFloat(state, 0xe0u, 8.0f);
    SetFloat(state, 0xe4u, 9.0f);
    SetFloat(state, 0xe8u, 10.0f);
    return state;
}

struct RecordRuntime final : pal4::IMonsterCombatPositionRuntime {
    std::vector<std::string>* calls{};
    std::int32_t count{2};
    std::vector<std::array<std::uint32_t, 3>> records;
    std::vector<std::uint32_t> ends;
    pal4::MonsterActionDispatchRuntimeStub action;

    explicit RecordRuntime(std::vector<std::string>* log) : calls(log) {}

    std::int32_t PositionCount(std::uint32_t source) override {
        Check(source == 0x6000u, "candidate position count source");
        calls->emplace_back("count");
        return count;
    }
    std::uint32_t PositionId(std::uint32_t source,
                             std::uint32_t index) override {
        Check(source == 0x6000u && index < 2u, "candidate position ID");
        calls->emplace_back("id");
        return 0x1100u + index;
    }
    std::int32_t PositionValue(std::uint32_t source,
        const std::string& key, std::int32_t fallback) override {
        Check(source == 0x6000u && fallback == 0 &&
                  (key == "AAB_MST_ESSBEH_combat_pos_0" ||
                   key == "AAB_MST_ESSBEH_combat_pos_1"),
              "candidate indexed combat key");
        calls->emplace_back("value");
        return key.back() == '0' ? -2 : 7;
    }
    void AppendRecord(State& state, std::uint32_t array_address,
        std::uint32_t end_address, std::uint32_t count_value,
        const std::array<std::uint32_t, 3>& record) override {
        Check(array_address == 0x5710u && count_value == 1u,
              "manager combat array address and insertion count");
        calls->emplace_back("append");
        ends.push_back(end_address);
        records.push_back(record);
        state.words[0x718u / 4u] += 12u;
    }
    void AddManagerText(std::uint32_t manager,
                        std::uint32_t text_address) override {
        Check(manager == 0x5000u && text_address == 0xbbbbu,
              "candidate text forwarded to manager");
        calls->emplace_back("text");
    }
    pal4::IMonsterActionDispatchRuntime& ActionRuntime() override {
        return action;
    }
};

struct Runtime final : pal4::IMonsterCombatInteractionRuntime {
    std::vector<std::string> calls;
    RecordRuntime records{&calls};
    pal4::MonsterCombatCandidate candidate{};
    std::string script;
    double vertical_property{2.0};
    double distance{5.0};
    float radius{5.0f};
    bool blocked{};
    float observed_dx{};
    float observed_dz{};
    std::array<float, 6> observed_endpoints{};

    Runtime() {
        candidate.active = true;
        candidate.pursuit_mode = 1u;
        candidate.source_address = 0x6000u;
        candidate.text_address = 0xbbbbu;
        candidate.timer_word = 0x1234u;
        candidate.position = {4.0f, 4.0f, 7.0f};
        candidate.collision_position = {11.0f, 12.0f, 13.0f};
    }

    pal4::MonsterCombatCandidate Candidate(std::uint32_t address) override {
        Check(address == 0x6000u, "combat candidate address");
        calls.emplace_back("candidate");
        return candidate;
    }
    std::string ScriptFunction(std::uint32_t source) override {
        Check(source == 0x6000u, "script query candidate source");
        calls.emplace_back("script");
        return script;
    }
    double TargetDoubleProperty(std::uint32_t address) override {
        Check(address == 0x7000u, "combat target property address");
        calls.emplace_back("target-property");
        return vertical_property;
    }
    double PlanarDistance(float dx, float dz) override {
        calls.emplace_back("distance");
        observed_dx = dx;
        observed_dz = dz;
        return distance;
    }
    float InteractionRadius(std::uint32_t source) override {
        Check(source == 0x8000u, "manager combat radius source");
        calls.emplace_back("radius");
        return radius;
    }
    std::uint32_t CollisionClump() override {
        calls.emplace_back("clump");
        return 0x9000u;
    }
    bool CollisionBlocked(const std::array<float, 6>& endpoints,
                          std::uint32_t clump) override {
        Check(clump == 0x9000u, "combat clump address");
        calls.emplace_back("collision");
        observed_endpoints = endpoints;
        return blocked;
    }
    pal4::IMonsterCombatPositionRuntime& Records() override {
        calls.emplace_back("records");
        return records;
    }
};

std::uint32_t Process(State& state, Runtime& runtime,
                      std::uint32_t candidate = 0x6000u,
                      std::uint32_t manager = 0x5000u) {
    return pal4::MonsterCombatInteractionCallback::Process(
        state, candidate, manager, runtime);
}

void InitialAndScriptGates() {
    auto state = InitialState();
    Runtime runtime;
    state.words[0x6e4u / 4u] = 0u;
    Check(Process(state, runtime) == 0x6000u && runtime.calls.empty(),
          "missing target returns candidate before object reads");
    state.words[0x6e4u / 4u] = 0x7000u;
    Check(Process(state, runtime, 0x5000u) == 0x5000u &&
              runtime.calls.empty(),
          "self candidate skips combat interaction");
    runtime.candidate.active = false;
    Check(Process(state, runtime) == 0x6000u &&
              runtime.calls == std::vector<std::string>{"candidate"},
          "inactive candidate stops before script check");

    Runtime wrong_mode;
    wrong_mode.candidate.pursuit_mode = 2u;
    Check(Process(state, wrong_mode) == 0x6000u &&
              wrong_mode.calls == std::vector<std::string>{"candidate"},
          "only pursuit mode one reaches script gate");
    Runtime named;
    named.script = "fight";
    Check(Process(state, named) == 0x6000u &&
              named.calls == std::vector<std::string>{
                  "candidate", "script"},
          "configured script function prevents combat handoff");
}

void GeometryAndCollisionGates() {
    for (const auto property : {1.9}) {
        auto state = InitialState();
        Runtime runtime;
        runtime.vertical_property = property;
        Check(Process(state, runtime) == 0x6000u &&
                  runtime.calls.back() == "target-property",
              "vertical property below height difference exits early");
    }
    for (const auto distance : {5.01,
             std::numeric_limits<double>::quiet_NaN()}) {
        auto state = InitialState();
        Runtime runtime;
        runtime.distance = distance;
        Check(Process(state, runtime) == 0x6000u &&
                  runtime.observed_dx == 3.0f &&
                  runtime.observed_dz == 4.0f &&
                  runtime.calls.back() == "radius",
              "outside or unordered planar distance exits before collision");
    }
    auto state = InitialState();
    Runtime blocked;
    blocked.blocked = true;
    Check(Process(state, blocked) == 0x6000u &&
              blocked.calls.back() == "collision" &&
              blocked.observed_endpoints == std::array<float, 6>{
                  8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f},
          "blocked collision keeps manager/candidate endpoint order");
}

void CombatRecordsAndText() {
    auto state = InitialState();
    Runtime runtime;
    runtime.vertical_property = std::numeric_limits<double>::quiet_NaN();
    Check(Process(state, runtime) == 0x6000u &&
              runtime.records.ends == std::vector<std::uint32_t>{
                  0xa000u, 0xa00cu} &&
              runtime.records.records ==
                  std::vector<std::array<std::uint32_t, 3>>{
                      {0x1100u, 0xfffffffeu, 0x1234u},
                      {0x1101u, 7u, 0x1234u}} &&
              runtime.calls == std::vector<std::string>{
                  "candidate", "script", "target-property", "distance",
                  "radius", "clump", "collision", "records",
                  "count", "id", "value", "append",
                  "count", "id", "value", "append", "count", "text"},
          "unordered vertical comparison proceeds to records and text");

    auto empty_state = InitialState();
    Runtime empty;
    empty.records.count = 0;
    Check(Process(empty_state, empty) == 0x6000u &&
              empty.records.records.empty() &&
              empty.calls.back() == "text",
          "zero positions still forward candidate text");

    pal4::MonsterCombatInteractionRuntimeStub stub;
    Check(pal4::MonsterCombatInteractionCallback::Process(
              empty_state, 0x6000u, 0x5000u, stub) == 0x6000u,
          "candidate, XML, collision and record services have stubs");
}

}  // namespace

int main() {
    InitialAndScriptGates();
    GeometryAndCollisionGates();
    CombatRecordsAndText();
    std::cout << "monster combat interaction cases passed\n";
}
