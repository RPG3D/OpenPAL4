#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_path_endpoint_save.h"

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

struct Runtime final : pal4::IMonsterPathEndpointSaveRuntime {
    std::vector<std::string> calls;
    std::vector<std::pair<std::string, std::int32_t>> integers;
    std::vector<std::pair<std::string, float>> floats;
    std::vector<std::string> actions;
    std::uint32_t first_result{0x1234u};

    std::int32_t ScriptVersion() override {
        calls.emplace_back("version");
        return 42;
    }
    void WriteMemoryInt(std::uint32_t memory, std::string_view key,
                        std::int32_t value) override {
        Check(memory == 0x9000u, "runtime memory object");
        calls.emplace_back("write-int");
        integers.emplace_back(std::string(key), value);
    }
    void WriteMemoryFloat(std::uint32_t memory, std::string_view key,
                          float value) override {
        Check(memory == 0x9000u, "runtime memory object");
        calls.emplace_back("write-float");
        floats.emplace_back(std::string(key), value);
    }
    float HelperSpeed(std::uint32_t helper) override {
        Check(helper == 0x6000u, "path helper speed source");
        calls.emplace_back("speed");
        return 3.25f;
    }
    float AnimationTime(std::uint32_t animation) override {
        Check(animation == 0x5170u, "animation time source");
        calls.emplace_back("time");
        return 1.75f;
    }
    void RemoveGridElement(std::uint32_t grid,
                           std::uint32_t helper) override {
        Check(grid == 0x7000u && helper == 0x6000u,
              "grid removal addresses");
        calls.emplace_back("remove-grid");
    }
    std::uint32_t SelectAction(std::uint32_t animation,
                               std::string_view name,
                               float blend) override {
        Check(animation == 0x5170u && blend == 0.5f,
              "animation action address and blend");
        calls.emplace_back("action");
        actions.emplace_back(name);
        return name == "C01" ? first_result : 0x5678u;
    }
    std::uint32_t SetAnimationFlag(std::uint32_t,
                                   std::uint32_t) override {
        Check(false, "C02 release must not set animation flag");
        return 0u;
    }
};

pal4::MonsterBehaviorManagerState Manager() {
    pal4::MonsterBehaviorManagerState manager{};
    manager.words[0x204u / 4u] = 0x7000u;
    manager.words[0x668u / 4u] = 0x9000u;
    return manager;
}

pal4::MonsterPathEndpointTargetState Target() {
    pal4::MonsterPathEndpointTargetState target{};
    target.words[0x1cu / 4u] = 0x5000u;
    return target;
}

void SavesFieldsAndKeepsFirstAction() {
    const auto manager = Manager();
    const auto target = Target();
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xc0u / 4u] = 0xffffffffu;
    helper.words[0xc4u / 4u] = 0x11223302u;
    helper.words[0xb8u / 4u] = 0xaabbcc01u;
    Runtime runtime;
    Check(pal4::MonsterPathEndpointSave::Release(
              target, manager, helper, 0x6000u, runtime) == 0x1234u &&
              runtime.integers ==
                  std::vector<std::pair<std::string, std::int32_t>>{
                      {"AAB_MST_memory_last_behavior_id", 42},
                      {"AABPedestrian_MST_memory_pathDirection", -1},
                      {"AABPedestrian_MST_memory_wander", 2}} &&
              runtime.floats ==
                  std::vector<std::pair<std::string, float>>{
                      {"AAB_MST_memory_speed", 3.25f},
                      {"AAB_MST_memory_actiontime", 1.75f}} &&
              helper.words[0xb8u / 4u] == 0xaabbcc00u &&
              helper.words[0xc4u / 4u] == 0x11223302u &&
              runtime.actions == std::vector<std::string>{"C01"} &&
              runtime.calls == std::vector<std::string>{
                  "version", "write-int", "speed", "write-float",
                  "time", "write-float", "write-int", "write-int",
                  "remove-grid", "action"},
          "release saves five fields then removes grid and starts C01");
}

void FallsBackToZ01() {
    const auto manager = Manager();
    const auto target = Target();
    pal4::BehaviorPathEndpointState helper{};
    Runtime runtime;
    runtime.first_result = 0u;
    Check(pal4::MonsterPathEndpointSave::Release(
              target, manager, helper, 0x6000u, runtime) == 0x5678u &&
              runtime.actions == std::vector<std::string>{"C01", "Z01"},
          "failed C01 action falls back to Z01 and returns its result");
}

void LinkableStub() {
    const auto manager = Manager();
    const auto target = Target();
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 1u;
    pal4::MonsterPathEndpointSaveRuntimeStub runtime;
    Check(pal4::MonsterPathEndpointSave::Release(
              target, manager, helper, 0x6000u, runtime) == 0u &&
              helper.words[0xb8u / 4u] == 0u,
          "XML, grid and animation boundaries link with no-op stubs");
}

}  // namespace

int main() {
    SavesFieldsAndKeepsFirstAction();
    FallsBackToZ01();
    LinkableStub();
    std::cout << "monster path endpoint save cases passed\n";
}
