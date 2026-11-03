#include "pal4/monster_animation_flag_save.h"
#include "pal4/monster_behavior_manager.h"

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

struct Runtime final : pal4::IMonsterAnimationFlagSaveRuntime {
    std::vector<std::string> calls;
    std::uint32_t first_result{};
    pal4::BehaviorPathEndpointState* helper{};

    std::int32_t ScriptVersion() override {
        calls.emplace_back("version");
        return 2;
    }
    void WriteMemoryInt(std::uint32_t memory,
                        std::string_view key, std::int32_t value) override {
        Check(memory == 0x9000u &&
              key == "AAB_MST_memory_last_behavior_id" && value == 2,
              "behavior version save");
        calls.emplace_back("write-version");
    }
    float AnimationTime(std::uint32_t animation) override {
        Check(animation == 0x5170u, "animation time source");
        calls.emplace_back("time");
        return 1.75f;
    }
    void WriteMemoryFloat(std::uint32_t memory,
                          std::string_view key, float value) override {
        Check(memory == 0x9000u &&
              key == "AAB_MST_memory_actiontime" && value == 1.75f,
              "animation time save");
        calls.emplace_back("write-time");
    }
    void RemoveGridElement(std::uint32_t grid,
                           std::uint32_t address) override {
        Check(grid == 0x7000u && address == 0x6000u &&
              helper->words[0xb8u / 4u] == 0xaabbcc00u,
              "clear active flag before grid removal");
        calls.emplace_back("remove-grid");
    }
    std::uint32_t SelectAnimation(std::uint32_t animation,
                                  std::string_view name) override {
        Check(animation == 0x5170u, "class-two manager pointer offset");
        calls.emplace_back(std::string(name));
        return name == "C01" ? first_result : 0x55u;
    }
};

void Run(bool first_succeeds) {
    pal4::MonsterPathEndpointTargetState target{};
    target.words[0x0cu / 4u] = 0x5000u;
    target.words[0x1cu / 4u] = 0xdead0000u;
    pal4::MonsterBehaviorManagerState manager{};
    manager.words[0x204u / 4u] = 0x7000u;
    manager.words[0x668u / 4u] = 0x9000u;
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 0xaabbcc01u;
    Runtime runtime;
    runtime.helper = &helper;
    runtime.first_result = first_succeeds ? 0x77u : 0u;
    const auto result = pal4::MonsterAnimationFlagSave::Release(
        target, manager, helper, 0x6000u, runtime);
    auto expected = std::vector<std::string>{
        "version", "write-version", "time", "write-time",
        "remove-grid", "C01"};
    if (!first_succeeds)
        expected.emplace_back("Z01");
    Check(result == (first_succeeds ? 0x77u : 0x55u) &&
          runtime.calls == expected &&
          helper.words[0xb8u / 4u] == 0xaabbcc00u,
          "animation target saves and releases in original order");
}

void LinkableStub() {
    pal4::MonsterPathEndpointTargetState target{};
    pal4::MonsterBehaviorManagerState manager{};
    pal4::BehaviorPathEndpointState helper{};
    pal4::MonsterAnimationFlagSaveRuntimeStub runtime;
    Check(pal4::MonsterAnimationFlagSave::Release(
              target, manager, helper, 0u, runtime) == 0u,
          "animation target default runtime links");
}

}  // namespace

int main() {
    Run(true);
    Run(false);
    LinkableStub();
    std::cout << "monster animation flag save cases passed\n";
}
