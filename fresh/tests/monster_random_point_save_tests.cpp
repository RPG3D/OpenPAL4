#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_random_point_save.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMonsterPathEndpointSaveRuntime,
                       pal4::IMonsterRandomPointRuntime {
    std::vector<std::string> calls;
    std::vector<std::pair<std::string, std::int32_t>> ints;
    std::vector<std::pair<std::string, float>> floats;
    std::uint32_t first_result{};
    pal4::BehaviorPathEndpointState* helper{};

    std::int32_t ScriptVersion() override {
        Check(false, "point save uses sound runtime type");
        return 0;
    }
    std::int32_t SoundRuntimeType() override {
        calls.emplace_back("type");
        return 3;
    }
    void WriteMemoryInt(std::uint32_t memory,
                        std::string_view key, std::int32_t value) override {
        Check(memory == 0x9000u, "memory int address");
        calls.emplace_back("write-int");
        ints.emplace_back(std::string(key), value);
    }
    void WriteMemoryFloat(std::uint32_t memory,
                          std::string_view key, float value) override {
        Check(memory == 0x9000u, "memory float address");
        calls.emplace_back("write-float");
        floats.emplace_back(std::string(key), value);
    }
    float HelperSpeed(std::uint32_t address) override {
        Check(address == 0x6000u, "speed helper address");
        calls.emplace_back("speed");
        return 3.25f;
    }
    float AnimationTime(std::uint32_t address) override {
        Check(address == 0x5170u, "animation time address");
        calls.emplace_back("time");
        return 1.75f;
    }
    void RemoveGridElement(std::uint32_t grid,
                           std::uint32_t address) override {
        Check(grid == 0x7000u && address == 0x6000u &&
              helper->words[0xb8u / 4u] == 0xaabbcc00u,
              "active byte clears before grid removal");
        calls.emplace_back("remove-grid");
    }
    std::uint32_t SelectAction(std::uint32_t address,
                               std::string_view name, float blend) override {
        Check(address == 0x5170u && blend == 0.5f,
              "animation action address and blend");
        calls.emplace_back(std::string(name));
        return name == "C01" ? first_result : 0x55u;
    }
    std::uint32_t SetAnimationFlag(std::uint32_t,
                                   std::uint32_t) override {
        Check(false, "point save does not set animation flag");
        return 0u;
    }
    pal4::BehaviorVector3 CurrentHelperPoint(
        std::uint32_t) override {
        Check(false, "point save reads stored helper fields directly");
        return {};
    }
    float ReadSoundActionTime(std::uint32_t, float) override {
        Check(false, "point save reads animation time directly");
        return 0.0f;
    }
};

void Run(bool first_succeeds) {
    pal4::MonsterPathEndpointTargetState target{};
    target.words[0x04u / 4u] = 0x5000u;
    target.words[0x1cu / 4u] = 0xdead0000u;
    pal4::MonsterBehaviorManagerState manager{};
    manager.words[0x204u / 4u] = 0x7000u;
    manager.words[0x668u / 4u] = 0x9000u;
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 0xaabbcc01u;
    helper.words[0xd4u / 4u] = std::bit_cast<std::uint32_t>(10.0f);
    helper.words[0xd8u / 4u] = std::bit_cast<std::uint32_t>(-20.0f);
    helper.words[0xdcu / 4u] = std::bit_cast<std::uint32_t>(30.0f);
    Runtime runtime;
    runtime.helper = &helper;
    runtime.first_result = first_succeeds ? 0x77u : 0u;

    const auto result = pal4::MonsterRandomPointSave::Release(
        target, manager, helper, 0x6000u, runtime, runtime);
    const auto expected_floats =
        std::vector<std::pair<std::string, float>>{
            {"AAB_MST_memory_speed", 3.25f},
            {"AAB_MST_memory_actiontime", 1.75f},
            {"AABWander_MST_memory_start_x", 10.0f},
            {"AABWander_MST_memory_start_y", -20.0f},
            {"AABWander_MST_memory_start_z", 30.0f}};
    auto expected_calls = std::vector<std::string>{
        "type", "write-int", "speed", "write-float", "time",
        "write-float", "write-float", "write-float", "write-float",
        "remove-grid", "C01"};
    if (!first_succeeds)
        expected_calls.emplace_back("Z01");
    Check(result == (first_succeeds ? 0x77u : 0x55u) &&
          runtime.ints ==
              std::vector<std::pair<std::string, std::int32_t>>{
                  {"AAB_MST_memory_last_behavior_id", 3}} &&
          runtime.floats == expected_floats &&
          runtime.calls == expected_calls &&
          helper.words[0xb8u / 4u] == 0xaabbcc00u,
          "point save fields, order, animation fallback and return");
}

void LinkableStubs() {
    pal4::MonsterPathEndpointTargetState target{};
    pal4::MonsterBehaviorManagerState manager{};
    pal4::BehaviorPathEndpointState helper{};
    pal4::MonsterPathEndpointSaveRuntimeStub runtime;
    pal4::MonsterRandomPointRuntimeStub point_runtime;
    Check(pal4::MonsterRandomPointSave::Release(
              target, manager, helper, 0u, runtime, point_runtime) == 0u,
          "point save default runtimes link");
}

}  // namespace

int main() {
    Run(true);
    Run(false);
    LinkableStubs();
    std::cout << "monster random point save cases passed\n";
}
