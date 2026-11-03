#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_path_endpoint_save.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
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
    std::vector<std::string> writes;
    std::uint32_t first_result{};
    std::uint32_t flag_result{0x1234u};
    pal4::BehaviorPathEndpointState* helper{};

    std::int32_t ScriptVersion() override {
        calls.emplace_back("version");
        return 1;
    }
    void WriteMemoryInt(std::uint32_t memory,
                        std::string_view key, std::int32_t value) override {
        Check(memory == 0x9000u, "C03 memory address");
        calls.emplace_back("write-int");
        writes.emplace_back(std::string(key) + "=" + std::to_string(value));
    }
    void WriteMemoryFloat(std::uint32_t memory,
                          std::string_view key, float value) override {
        Check(memory == 0x9000u, "C03 float memory address");
        calls.emplace_back("write-float");
        writes.emplace_back(std::string(key) + "=" + std::to_string(value));
    }
    float HelperSpeed(std::uint32_t address) override {
        Check(address == 0x6000u, "C03 speed helper address");
        calls.emplace_back("speed");
        return 2.5f;
    }
    float AnimationTime(std::uint32_t address) override {
        Check(address == 0x5170u, "C03 animation address");
        calls.emplace_back("time");
        return 3.25f;
    }
    void RemoveGridElement(std::uint32_t grid,
                           std::uint32_t address) override {
        Check(grid == 0x7000u && address == 0x6000u,
              "C03 grid removal addresses");
        Check((helper->words[0xb8u / 4u] & 0xffu) == 0u,
              "active byte clears before grid removal");
        calls.emplace_back("remove");
    }
    std::uint32_t SelectAction(std::uint32_t address,
                               std::string_view name, float blend) override {
        Check(address == 0x5170u && blend == 0.5f,
              "C03 action arguments");
        calls.emplace_back(std::string(name));
        return name == "C01" ? first_result : 0x55u;
    }
    std::uint32_t SetAnimationFlag(std::uint32_t address,
                                   std::uint32_t flag) override {
        Check(address == 0x5170u && flag == 1u,
              "C03 final animation flag");
        calls.emplace_back("flag");
        return flag_result;
    }
};

void Run(bool first_succeeds) {
    pal4::MonsterPathEndpointTargetState target{};
    target.words[0x14u / 4u] = 0x5000u;
    target.words[0x1cu / 4u] = 0xdead0000u;
    pal4::MonsterBehaviorManagerState manager{};
    manager.words[0x204u / 4u] = 0x7000u;
    manager.words[0x668u / 4u] = 0x9000u;
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 0xaabbcc01u;
    helper.words[0xc0u / 4u] = 0xfffffffeu;
    helper.words[0xc4u / 4u] = 0x112233feu;
    Runtime runtime;
    runtime.helper = &helper;
    runtime.first_result = first_succeeds ? 0x77u : 0u;

    const auto result = pal4::MonsterPathEndpointC03Save::Release(
        target, manager, helper, 0x6000u, runtime);
    Check(result == runtime.flag_result &&
          helper.words[0xb8u / 4u] == 0xaabbcc00u &&
          runtime.writes == std::vector<std::string>{
              "AAB_MST_memory_last_behavior_id=1",
              "AAB_MST_memory_speed=2.500000",
              "AAB_MST_memory_actiontime=3.250000",
              "AABPedestrian_MST_memory_pathDirection=-2",
              "AABPedestrian_MST_memory_wander=254"},
          "C03 saves values and returns animation flag result");
    auto expected = std::vector<std::string>{
        "version", "write-int", "speed", "write-float", "time",
        "write-float", "write-int", "write-int", "remove", "C01"};
    if (!first_succeeds)
        expected.emplace_back("Z01");
    expected.emplace_back("flag");
    Check(runtime.calls == expected, "C03 save call order and action fallback");
}

void LinkableStub() {
    pal4::MonsterPathEndpointTargetState target{};
    pal4::MonsterBehaviorManagerState manager{};
    pal4::BehaviorPathEndpointState helper{};
    pal4::MonsterPathEndpointSaveRuntimeStub runtime;
    Check(pal4::MonsterPathEndpointC03Save::Release(
              target, manager, helper, 0u, runtime) == 0u,
          "C03 save default runtime links");
}

}  // namespace

int main() {
    Run(true);
    Run(false);
    LinkableStub();
    std::cout << "monster path endpoint C03 save cases passed\n";
}
