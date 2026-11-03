#include "pal4/npc_animation_flag_restore.h"

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

struct Runtime final : pal4::NpcPathEndpointRestoreRuntimeStub {
    std::vector<std::string> calls;
    std::vector<std::string> animations;
    std::vector<std::pair<std::uint32_t, float>> scalars;
    std::vector<std::pair<std::uint32_t, pal4::BehaviorVector3>> vectors;
    bool c13_succeeds{};
    bool c01_succeeds{};
    std::int32_t last_behavior{0x7fffffff};
    std::int32_t version{2};
    float action_time{3.25f};
    float restored_time{-1.0f};
    std::uint32_t path{0xffffffffu};
    std::uint32_t parameter{};
    std::uint32_t flag{};
    std::uint32_t completion_flag{};

    std::uint32_t GridParameter(std::uint32_t grid) override {
        Check(grid == 0x7000u, "grid pointer");
        calls.emplace_back("grid-parameter");
        return 0x7020u;
    }
    void ConfigurePath(pal4::BehaviorPathEndpointState&,
                       std::uint32_t helper, std::uint32_t grid_parameter,
                       std::uint32_t path_address) override {
        Check(helper == 0x6000u, "configure helper address");
        calls.emplace_back("configure-path");
        parameter = grid_parameter;
        path = path_address;
    }
    void SetHelperScalar(std::uint32_t helper,
                         std::uint32_t slot, float value) override {
        Check(helper == 0x6000u, "scalar helper address");
        calls.emplace_back("scalar");
        scalars.emplace_back(slot, value);
    }
    pal4::MonsterTransformRows ActorTransform(std::uint32_t actor) override {
        Check(actor == 0xa000u, "actor component pointer");
        calls.emplace_back("transform");
        return {{{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f},
                 {7.0f, 8.0f, 9.0f}, {10.0f, 11.0f, 12.0f}}};
    }
    void SetHelperVector(std::uint32_t helper, std::uint32_t slot,
                         const pal4::BehaviorVector3& value) override {
        Check(helper == 0x6000u, "vector helper address");
        calls.emplace_back("vector");
        vectors.emplace_back(slot, value);
    }
    void AddGridElement(std::uint32_t grid,
                        std::uint32_t helper) override {
        Check(grid == 0x7000u && helper == 0x6000u,
              "grid insertion arguments");
        calls.emplace_back("add-grid");
    }
    bool SelectAnimation(std::uint32_t animation,
                         std::string_view name) override {
        Check(animation == 0x5170u, "target manager animation offset");
        calls.emplace_back("select-animation");
        animations.emplace_back(name);
        if (name == "C13") return c13_succeeds;
        if (name == "C01") return c01_succeeds;
        return true;
    }
    void SetAnimationFlag(std::uint32_t animation,
                          std::uint32_t value) override {
        Check(animation == 0x5170u, "animation flag address");
        calls.emplace_back("animation-flag");
        flag = value;
    }
    std::int32_t ReadMemoryInt(std::uint32_t memory,
                              std::string_view key,
                              std::int32_t fallback) override {
        Check(memory == 0x9000u &&
              key == "AAB_NPC_memory_last_behavior_id" &&
              fallback == 0x7fffffff, "last behavior lookup");
        calls.emplace_back("read-behavior");
        return last_behavior;
    }
    std::int32_t ScriptVersion() override {
        calls.emplace_back("version");
        return version;
    }
    float ReadMemoryFloat(std::uint32_t memory,
                          std::string_view key, float fallback) override {
        Check(memory == 0x9000u &&
              key == "AAB_NPC_memory_actiontime" && fallback == 0.0f,
              "action time lookup");
        calls.emplace_back("read-time");
        return action_time;
    }
    std::uint32_t SetAnimationTimeAndReturn(
        std::uint32_t animation, float time) override {
        Check(animation == 0x5170u, "animation time address");
        calls.emplace_back("set-time");
        restored_time = time;
        return 0x1234u;
    }
    std::uint32_t AnimationFlag(std::uint32_t animation) override {
        Check(animation == 0x5170u, "completion animation address");
        calls.emplace_back("get-flag");
        return completion_flag;
    }
};

pal4::GameObjectState Manager() {
    pal4::GameObjectState manager{};
    manager.words[0x204u / 4u] = 0x7000u;
    manager.words[0x668u / 4u] = 0x9000u;
    manager.words[0x14u / 4u] = 0xa000u;
    manager.words[0xdcu / 4u] = std::bit_cast<std::uint32_t>(2.0f);
    return manager;
}

void Restore(bool c13, bool c01) {
    pal4::NpcPathEndpointTargetState target{};
    target.words[0x14u / 4u] = 0x5000u;
    target.words[0x24u / 4u] = 0xdeadbeefu;
    const auto manager = Manager();
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 0xaabbcc00u;
    Runtime runtime;
    runtime.c13_succeeds = c13;
    runtime.c01_succeeds = c01;
    runtime.last_behavior = 2;

    Check(pal4::NpcAnimationFlagRestore::Accept(
              target, manager, helper, 0x6000u, runtime) == 0x1234u,
          "matching behavior propagates action-time call result");
    Check(runtime.parameter == 0x7020u && runtime.path == 0u &&
          runtime.scalars ==
              std::vector<std::pair<std::uint32_t, float>>{
                  {0x68u, 0.0f}, {0x5cu, 3.0f}} &&
          runtime.vectors ==
              std::vector<std::pair<std::uint32_t, pal4::BehaviorVector3>>{
                  {0x14u, {7.0f, 8.0f, 9.0f}},
                  {0x04u, {1.0f, 2.0f, 3.0f}},
                  {0x0cu, {4.0f, 5.0f, 6.0f}},
                  {0x1cu, {10.0f, 11.0f, 12.0f}}},
          "path, scalar and transform restoration order");
    auto expected = std::vector<std::string>{"C13"};
    if (!c13) expected.emplace_back("C01");
    if (!c13 && !c01) expected.emplace_back("Z01");
    Check(runtime.animations == expected && runtime.flag == 1u &&
          runtime.restored_time == 3.25f &&
          helper.words[0xb8u / 4u] == 0xaabbcc01u,
          "animation fallback, flag and helper active byte");
    Check(runtime.calls[runtime.calls.size() - 3u] == "version" &&
          runtime.calls[runtime.calls.size() - 2u] == "read-time" &&
          runtime.calls.back() == "set-time",
          "behavior comparison precedes action-time restore");
}

void BehaviorMismatch() {
    pal4::NpcPathEndpointTargetState target{};
    target.words[0x14u / 4u] = 0x5000u;
    const auto manager = Manager();
    pal4::BehaviorPathEndpointState helper{};
    Runtime runtime;
    runtime.version = 7;
    Check(pal4::NpcAnimationFlagRestore::Accept(
              target, manager, helper, 0x6000u, runtime) == 7u &&
          runtime.restored_time == -1.0f &&
          runtime.calls.back() == "version",
          "missing behavior returns version without reading action time");
    runtime.last_behavior = 7;
    runtime.calls.clear();
    Check(pal4::NpcAnimationFlagRestore::Accept(
              target, manager, helper, 0x6000u, runtime) == 0x1234u &&
          runtime.restored_time == 3.25f,
          "version result is obtained from the runtime helper");
}

void CompletionPredicate() {
    pal4::NpcPathEndpointTargetState target{};
    target.words[0x14u / 4u] = 0x5000u;
    Runtime runtime;
    Check(!pal4::NpcAnimationFlagRestore::IsDone(target, runtime),
          "zero animation flag is incomplete");
    runtime.completion_flag = 0x100u;
    Check(pal4::NpcAnimationFlagRestore::IsDone(target, runtime) &&
          runtime.calls == std::vector<std::string>{"get-flag", "get-flag"},
          "nonzero animation flag completes target without other calls");
}

}  // namespace

int main() {
    Restore(true, false);
    Restore(false, true);
    Restore(false, false);
    BehaviorMismatch();
    CompletionPredicate();
    std::cout << "NPC animation restore cases passed\n";
}
