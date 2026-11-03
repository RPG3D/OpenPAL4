#include "pal4/npc_random_point_save.h"

#include <bit>
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

struct Runtime final : pal4::MonsterPathEndpointSaveRuntimeStub,
                       pal4::IMonsterRandomPointRuntime {
    std::vector<std::string> calls;
    pal4::BehaviorPathEndpointState* helper{};
    std::uint32_t first_result{};

    std::int32_t SoundRuntimeType() override {
        calls.emplace_back("type");
        return 3;
    }
    pal4::BehaviorVector3 CurrentHelperPoint(std::uint32_t) override {
        Check(false, "save does not query current point");
        return {};
    }
    float ReadSoundActionTime(std::uint32_t, float) override {
        Check(false, "save does not read saved action time");
        return 0.0f;
    }
    void WriteMemoryInt(std::uint32_t memory,
                        std::string_view key, std::int32_t value) override {
        Check(memory == 0x9000u &&
              key == "AAB_NPC_memory_last_behavior_id" && value == 3,
              "NPC type write");
        calls.emplace_back("write-type");
    }
    float HelperSpeed(std::uint32_t helper_address) override {
        Check(helper_address == 0x6000u, "helper speed address");
        calls.emplace_back("speed");
        return 4.25f;
    }
    float AnimationTime(std::uint32_t animation) override {
        Check(animation == 0x5170u, "animation time address");
        calls.emplace_back("time");
        return 1.75f;
    }
    void WriteMemoryFloat(std::uint32_t memory,
                          std::string_view key, float value) override {
        Check(memory == 0x9000u, "NPC memory address");
        if (key == "AAB_NPC_memory_speed")
            Check(value == 4.25f, "saved speed");
        else if (key == "AAB_NPC_memory_actiontime")
            Check(value == 1.75f, "saved action time");
        else if (key == "AABWander_NPC_memory_start_x")
            Check(value == 10.0f, "saved point x");
        else if (key == "AABWander_NPC_memory_start_y")
            Check(value == 20.0f, "saved point y");
        else if (key == "AABWander_NPC_memory_start_z")
            Check(value == 30.0f, "saved point z");
        else
            Check(false, "unexpected NPC memory key");
        calls.emplace_back(key);
    }
    void RemoveGridElement(std::uint32_t grid,
                           std::uint32_t helper_address) override {
        Check(grid == 0x7000u && helper_address == 0x6000u &&
              helper->words[0xb8u / 4u] == 0xaabbcc00u,
              "clear active byte before grid removal");
        calls.emplace_back("remove-grid");
    }
    std::uint32_t SelectAction(std::uint32_t animation,
                               std::string_view name, float blend) override {
        Check(animation == 0x5170u && blend == 0.5f,
              "selection animation address and blend");
        calls.emplace_back(name);
        return name == "C01" ? first_result : 0x55u;
    }
};

void Release(bool first_succeeds) {
    pal4::NpcPathEndpointTargetState target{};
    target.words[0x0cu / 4u] = 0x5000u;
    target.words[0x24u / 4u] = 0xdeadu;
    pal4::GameObjectState manager{};
    manager.words[0x204u / 4u] = 0x7000u;
    manager.words[0x668u / 4u] = 0x9000u;
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 0xaabbcc01u;
    helper.words[0xd4u / 4u] = std::bit_cast<std::uint32_t>(10.0f);
    helper.words[0xd8u / 4u] = std::bit_cast<std::uint32_t>(20.0f);
    helper.words[0xdcu / 4u] = std::bit_cast<std::uint32_t>(30.0f);
    Runtime runtime;
    runtime.helper = &helper;
    runtime.first_result = first_succeeds ? 0x77u : 0u;
    const auto result = pal4::NpcRandomPointSave::Release(
        target, manager, helper, 0x6000u, runtime, runtime);
    auto expected = std::vector<std::string>{
        "type", "write-type", "speed", "AAB_NPC_memory_speed",
        "time", "AAB_NPC_memory_actiontime",
        "AABWander_NPC_memory_start_x",
        "AABWander_NPC_memory_start_y",
        "AABWander_NPC_memory_start_z", "remove-grid", "C01"};
    if (!first_succeeds) expected.emplace_back("Z01");
    Check(result == (first_succeeds ? 0x77u : 0x55u) &&
          runtime.calls == expected &&
          helper.words[0xb8u / 4u] == 0xaabbcc00u,
          "NPC wander save order, fallback and return value");
}

}  // namespace

int main() {
    Release(true);
    Release(false);
    std::cout << "NPC random point save cases passed\n";
}
