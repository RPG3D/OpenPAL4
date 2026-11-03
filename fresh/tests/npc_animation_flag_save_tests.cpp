#include "pal4/npc_animation_flag_save.h"

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
    pal4::BehaviorPathEndpointState* helper{};
    std::uint32_t c01_result{};

    std::int32_t ScriptVersion() override {
        calls.emplace_back("version");
        return 2;
    }
    void WriteMemoryInt(std::uint32_t memory,
                        std::string_view key, std::int32_t value) override {
        Check(memory == 0x9000u &&
              key == "AAB_NPC_memory_last_behavior_id" && value == 2,
              "NPC behavior ID write");
        calls.emplace_back("write-version");
    }
    float AnimationTime(std::uint32_t animation) override {
        Check(animation == 0x5170u, "target manager animation offset");
        calls.emplace_back("time");
        return 1.75f;
    }
    void WriteMemoryFloat(std::uint32_t memory,
                          std::string_view key, float value) override {
        Check(memory == 0x9000u &&
              key == "AAB_NPC_memory_actiontime" && value == 1.75f,
              "NPC action time write");
        calls.emplace_back("write-time");
    }
    void RemoveGridElement(std::uint32_t grid,
                           std::uint32_t address) override {
        Check(grid == 0x7000u && address == 0x6000u &&
              helper->words[0xb8u / 4u] == 0xaabbcc00u,
              "clear active byte before removing helper from grid");
        calls.emplace_back("remove-grid");
    }
    std::uint32_t SelectAnimation(std::uint32_t animation,
                                  std::string_view name) override {
        Check(animation == 0x5170u, "animation selection address");
        calls.emplace_back(name);
        return name == "C01" ? c01_result : 0x55u;
    }
};

void Release(bool c01_succeeds) {
    pal4::NpcPathEndpointTargetState target{};
    target.words[0x14u / 4u] = 0x5000u;
    target.words[0x24u / 4u] = 0xdeadu;
    pal4::GameObjectState manager{};
    manager.words[0x204u / 4u] = 0x7000u;
    manager.words[0x668u / 4u] = 0x9000u;
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 0xaabbcc01u;
    Runtime runtime;
    runtime.helper = &helper;
    runtime.c01_result = c01_succeeds ? 0x77u : 0u;
    const auto result = pal4::NpcAnimationFlagSave::Release(
        target, manager, helper, 0x6000u, runtime);
    auto expected = std::vector<std::string>{
        "version", "write-version", "time", "write-time",
        "remove-grid", "C01"};
    if (!c01_succeeds) expected.emplace_back("Z01");
    Check(result == (c01_succeeds ? 0x77u : 0x55u) &&
          runtime.calls == expected &&
          helper.words[0xb8u / 4u] == 0xaabbcc00u,
          "save order, animation fallback and raw return value");
}

void LinkableStub() {
    pal4::NpcPathEndpointTargetState target{};
    pal4::GameObjectState manager{};
    pal4::BehaviorPathEndpointState helper{};
    pal4::MonsterAnimationFlagSaveRuntimeStub runtime;
    Check(pal4::NpcAnimationFlagSave::Release(
              target, manager, helper, 0u, runtime) == 0u,
          "NPC animation save default runtime links");
}

}  // namespace

int main() {
    Release(true);
    Release(false);
    LinkableStub();
    std::cout << "NPC animation save cases passed\n";
}
