#include "pal4/npc_path_endpoint_save.h"

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

struct Runtime final : pal4::MonsterPathEndpointSaveRuntimeStub {
    std::vector<std::string> calls;
    std::vector<std::string> writes;
    pal4::BehaviorPathEndpointState* helper{};
    bool select_c01{};

    std::int32_t ScriptVersion() override {
        Check(false, "NPC uses the constant-zero class ID");
        return -1;
    }
    void WriteMemoryInt(std::uint32_t memory,
                         std::string_view key,
                         std::int32_t value) override {
        Check(memory == 0x9000u, "NPC memory address");
        calls.emplace_back("write-int");
        writes.push_back(std::string(key) + ":" + std::to_string(value));
    }
    void WriteMemoryFloat(std::uint32_t memory,
                           std::string_view key,
                           float value) override {
        Check(memory == 0x9000u, "NPC memory address");
        calls.emplace_back("write-float");
        writes.push_back(std::string(key) + ":" + std::to_string(value));
    }
    float HelperSpeed(std::uint32_t address) override {
        Check(address == 0x6000u, "helper speed address");
        calls.emplace_back("speed");
        return 4.0f;
    }
    float AnimationTime(std::uint32_t address) override {
        Check(address == 0x5170u, "animation time source");
        calls.emplace_back("time");
        return 2.5f;
    }
    void RemoveGridElement(std::uint32_t grid,
                            std::uint32_t address) override {
        Check(grid == 0x7000u && address == 0x6000u &&
              (helper->words[0xb8u / 4u] & 0xffu) == 0u,
              "grid removal follows active flag clear");
        calls.emplace_back("remove-grid");
    }
    std::uint32_t SelectAction(std::uint32_t address,
                               std::string_view name,
                               float blend) override {
        Check(address == 0x5170u && blend == 0.5f,
              "animation selection arguments");
        calls.push_back("select:" + std::string(name));
        return name == "C01" && select_c01 ? 1u : 0u;
    }
    std::uint32_t SetAnimationFlag(std::uint32_t address,
                                   std::uint32_t flag) override {
        Check(address == 0x5170u && flag == 1u,
              "final animation flag");
        calls.emplace_back("set-flag");
        return 0x5678u;
    }
};

void TestSave(bool first_action_available, bool c03) {
    pal4::NpcPathEndpointTargetState target{};
    target.words[(c03 ? 0x1cu : 0x24u) / 4u] = 0x5000u;
    target.words[(c03 ? 0x24u : 0x1cu) / 4u] = 0xdeadbeefu;
    pal4::GameObjectState manager{};
    manager.words[0x204u / 4u] = 0x7000u;
    manager.words[0x668u / 4u] = 0x9000u;
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 0xdeadbe01u;
    helper.words[0xc0u / 4u] = 0xfffffffeu;
    helper.words[0xc4u / 4u] = 0x11223380u;
    Runtime runtime;
    runtime.helper = &helper;
    runtime.select_c01 = first_action_available;
    const auto result = c03
        ? pal4::NpcPathEndpointC03Save::Release(
              target, manager, helper, 0x6000u, runtime)
        : pal4::NpcPathEndpointSave::Release(
              target, manager, helper, 0x6000u, runtime);
    Check(result == 0x5678u &&
          helper.words[0xb8u / 4u] == 0xdeadbe00u &&
          helper.words[0xc4u / 4u] == 0x11223380u,
          "NPC release preserves adjacent bytes and returns flag result");
    Check(runtime.writes == std::vector<std::string>{
              std::string("AAB_NPC_memory_last_behavior_id:") +
                  (c03 ? "1" : "0"),
              "AAB_NPC_memory_speed:4.000000",
              "AAB_NPC_memory_actiontime:2.500000",
              "AABPedestrian_NPC_memory_pathDirection:-2",
              "AABPedestrian_NPC_memory_wander:128"},
          "NPC runtime memory writes and byte-to-int conversion");
    const auto expected = first_action_available
        ? std::vector<std::string>{
            "write-int", "speed", "write-float", "time",
            "write-float", "write-int", "write-int", "remove-grid",
            "select:C01", "set-flag"}
        : std::vector<std::string>{
            "write-int", "speed", "write-float", "time",
            "write-float", "write-int", "write-int", "remove-grid",
            "select:C01", "select:Z01", "set-flag"};
    Check(runtime.calls == expected,
          "save, grid removal and animation selection order");
}

}  // namespace

int main() {
    TestSave(true, false);
    TestSave(false, false);
    TestSave(true, true);
    TestSave(false, true);
}
