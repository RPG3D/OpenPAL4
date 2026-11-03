#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_animation_flag_restore.h"
#include "pal4/monster_path_endpoint_restore.h"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

using NodeKey = std::pair<std::uint32_t, std::string>;

struct Runtime final : pal4::MonsterPathEndpointRestoreRuntimeStub {
    std::map<std::uint32_t, std::vector<std::uint32_t>> children;
    std::map<std::uint32_t, std::string> names;
    std::map<NodeKey, std::int32_t> integers;
    std::map<NodeKey, float> floats;
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, float>> scalars;
    std::vector<std::pair<std::uint32_t, pal4::BehaviorVector3>> vectors;
    std::vector<std::string> animations;
    std::string group_name;
    std::uint32_t configured_path{0xffffffffu};
    std::uint32_t configured_parameter{};
    std::uint32_t grid_element{};
    std::uint32_t animation_flag{};
    float animation_time{-1.0f};
    bool select_c02{true};
    bool select_x{true};
    bool select_c01{true};
    std::int32_t script_version{42};
    std::uint32_t completion_flag{};

    std::string ReadStringAttribute(std::uint32_t node,
        std::string_view key, std::string_view fallback) override {
        Check(node == 0x8000u &&
                  key == "MSTINFO_Behavior_GroupPathWayID" &&
                  fallback == "NOPathWay", "path attribute and default");
        calls.emplace_back("path-name");
        return group_name.empty() ? std::string(fallback) : group_name;
    }
    std::uint32_t ResolveWorldPath(std::string_view name) override {
        Check(name == (group_name.empty() ? "NOPathWay" : group_name),
              "path name is not converted to a number");
        calls.emplace_back("resolve-path");
        return 0xb000u;
    }
    std::uint32_t GridParameter(std::uint32_t grid) override {
        Check(grid == 0x7000u, "grid object address");
        calls.emplace_back("grid-parameter");
        return 0x7100u;
    }
    void ConfigurePath(pal4::BehaviorPathEndpointState&,
                       std::uint32_t helper, std::uint32_t grid_parameter,
                       std::uint32_t path) override {
        Check(helper == 0x6000u, "path helper address");
        calls.emplace_back("configure-path");
        configured_parameter = grid_parameter;
        configured_path = path;
    }
    std::int32_t ChildCount(std::uint32_t node) override {
        return static_cast<std::int32_t>(children[node].size());
    }
    std::uint32_t Child(std::uint32_t node,
                        std::uint32_t index) override {
        return children[node].at(index);
    }
    std::string ChildName(std::uint32_t node) override {
        return names[node];
    }
    std::int32_t ReadIntAttribute(std::uint32_t node,
        std::string_view key, std::int32_t fallback) override {
        const auto found = integers.find({node, std::string(key)});
        return found == integers.end() ? fallback : found->second;
    }
    float ReadFloatAttribute(std::uint32_t node,
        std::string_view key, float fallback) override {
        const auto found = floats.find({node, std::string(key)});
        return found == floats.end() ? fallback : found->second;
    }
    std::int32_t ReadMemoryInt(std::uint32_t memory,
        std::string_view key, std::int32_t fallback) override {
        Check(memory == 0x9000u, "manager memory map address");
        const auto found = integers.find({memory, std::string(key)});
        return found == integers.end() ? fallback : found->second;
    }
    float ReadMemoryFloat(std::uint32_t memory,
        std::string_view key, float fallback) override {
        Check(memory == 0x9000u, "manager memory map address");
        const auto found = floats.find({memory, std::string(key)});
        return found == floats.end() ? fallback : found->second;
    }
    void SetHelperScalar(std::uint32_t helper,
                         std::uint32_t slot, float value) override {
        Check(helper == 0x6000u, "scalar helper address");
        calls.emplace_back("scalar");
        scalars.emplace_back(slot, value);
    }
    pal4::MonsterTransformRows ActorTransform(
        std::uint32_t actor) override {
        Check(actor == 0xa000u, "actor component address");
        calls.emplace_back("transform");
        return {{{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f},
                 {7.0f, 8.0f, 9.0f}, {10.0f, 11.0f, 12.0f}}};
    }
    void SetHelperVector(std::uint32_t helper,
        std::uint32_t slot, const pal4::BehaviorVector3& value) override {
        Check(helper == 0x6000u, "vector helper address");
        calls.emplace_back("vector");
        vectors.emplace_back(slot, value);
    }
    void AddGridElement(std::uint32_t grid,
                        std::uint32_t helper) override {
        Check(grid == 0x7000u, "grid insertion address");
        calls.emplace_back("add-grid");
        grid_element = helper;
    }
    bool SelectAnimation(std::uint32_t address,
                         std::string_view name) override {
        Check(address == 0x5170u, "animation subobject address");
        calls.emplace_back("select");
        animations.emplace_back(name);
        if (name == "C02") return select_c02;
        if (name == "X") return select_x;
        if (name == "C01") return select_c01;
        return true;
    }
    void SetAnimationFlag(std::uint32_t address,
                          std::uint32_t flag) override {
        Check(address == 0x5170u, "animation flag address");
        calls.emplace_back("flag");
        animation_flag = flag;
    }
    std::int32_t ScriptVersion() override {
        calls.emplace_back("version");
        return script_version;
    }
    void SetAnimationTime(std::uint32_t address, float time) override {
        Check(address == 0x5170u, "animation time address");
        calls.emplace_back("time");
        animation_time = time;
    }
    std::uint32_t AnimationFlag(std::uint32_t address) override {
        Check(address == 0x5170u, "completion flag address");
        calls.emplace_back("completion-flag");
        return completion_flag;
    }
};

pal4::MonsterBehaviorManagerState Manager() {
    pal4::MonsterBehaviorManagerState manager{};
    manager.words[0x204u / 4u] = 0x7000u;
    manager.words[0x6d0u / 4u] = 0x8000u;
    manager.words[0x668u / 4u] = 0x9000u;
    manager.words[0x14u / 4u] = 0xa000u;
    manager.words[0xdcu / 4u] = std::bit_cast<std::uint32_t>(2.0f);
    return manager;
}

pal4::MonsterPathEndpointTargetState Target() {
    pal4::MonsterPathEndpointTargetState target{};
    target.words[0x1cu / 4u] = 0x5000u;
    return target;
}

void Accept(pal4::MonsterBehaviorManagerState& manager,
            pal4::BehaviorPathEndpointState& helper,
            Runtime& runtime) {
    auto target = Target();
    pal4::MonsterPathEndpointRestore::Accept(
        target, manager, helper, 0x6000u, runtime);
}

void NoParametersStillConfiguresPath() {
    auto manager = Manager();
    pal4::BehaviorPathEndpointState helper{};
    Runtime runtime;
    runtime.group_name = "m07-04";
    runtime.children[0x8000u] = {0x8100u};
    runtime.integers[{0x8100u, "MSTINFO_Behavior_ObjectType"}] = 2;
    Accept(manager, helper, runtime);
    Check(runtime.configured_path == 0xb000u &&
              runtime.configured_parameter == 0x7100u &&
              runtime.scalars.empty() && runtime.vectors.empty() &&
              runtime.grid_element == 0u &&
              runtime.calls == std::vector<std::string>{
                  "path-name", "resolve-path", "grid-parameter",
                  "configure-path"},
          "path is configured before object-type search and absent params stop");

    manager.words[0x6d0u / 4u] = 0u;
    Runtime missing_config;
    Accept(manager, helper, missing_config);
    Check(missing_config.configured_path == 0u &&
              missing_config.calls == std::vector<std::string>{
                  "grid-parameter", "configure-path"},
          "null config passes a null world path and stops after setup");
}

void FullRestoreAndAnimationFallback() {
    auto manager = Manager();
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 0xaabbcc00u;
    helper.words[0xc4u / 4u] = 0x11223344u;
    Runtime runtime;
    runtime.select_c02 = false;
    runtime.children[0x8000u] = {0x8100u, 0x8200u};
    runtime.integers[{0x8100u, "MSTINFO_Behavior_ObjectType"}] = 1;
    runtime.integers[{0x8200u, "MSTINFO_Behavior_ObjectType"}] = 0;
    runtime.children[0x8200u] = {0x8300u, 0x8400u};
    runtime.names[0x8300u] = "other";
    runtime.names[0x8400u] = "AAB_MST_parameters";
    runtime.floats[{0x8400u, "AABPedestrian_MST_walk_max_speed"}] = 12.5f;
    runtime.floats[{0x8400u, "AABPedestrian_MST_walk_max_force"}] = 7.0f;
    runtime.integers[{0x8400u, "AABPedestrian_MST_walk_pathfollow"}] = 2;
    runtime.floats[{0x9000u, "AAB_MST_memory_speed"}] = 3.5f;
    runtime.integers[{0x9000u,
        "AABPedestrian_MST_memory_pathDirection"}] = -1;
    runtime.integers[{0x9000u, "AABPedestrian_MST_memory_wander"}] = 1;
    runtime.integers[{0x9000u, "AAB_MST_memory_last_behavior_id"}] = 42;
    runtime.floats[{0x9000u, "AAB_MST_memory_actiontime"}] = 1.75f;
    Accept(manager, helper, runtime);

    Check(runtime.scalars ==
              std::vector<std::pair<std::uint32_t, float>>{
                  {0x7cu, 12.5f}, {0x74u, 7.0f}, {0x68u, 12.5f},
                  {0x5cu, 3.0f}, {0x68u, 3.5f}} &&
              helper.words[0xc0u / 4u] == 0xffffffffu &&
              helper.words[0xc4u / 4u] == 0x11220101u &&
              helper.words[0xb8u / 4u] == 0xaabbcc01u &&
              runtime.vectors ==
                  std::vector<std::pair<std::uint32_t,
                                        pal4::BehaviorVector3>>{
                      {0x14u, {7.0f, 8.0f, 9.0f}},
                      {0x04u, {1.0f, 2.0f, 3.0f}},
                      {0x0cu, {4.0f, 5.0f, 6.0f}},
                      {0x1cu, {10.0f, 11.0f, 12.0f}}} &&
              runtime.grid_element == 0x6000u &&
              runtime.animations ==
                  std::vector<std::string>{"C02", "Z02"} &&
              runtime.animation_flag == 1u &&
              runtime.animation_time == 1.75f,
          "restore applies parameters, preserved bytes, transform, grid and action time");

    auto position = [&](std::string_view call) {
        for (std::size_t i = 0; i < runtime.calls.size(); ++i)
            if (runtime.calls[i] == call) return i;
        return runtime.calls.size();
    };
    Check(position("configure-path") < position("scalar") &&
              position("scalar") < position("transform") &&
              position("transform") < position("vector") &&
              position("vector") < position("add-grid") &&
              position("add-grid") < position("select") &&
              position("flag") < position("version") &&
              position("version") < position("time"),
          "restore preserves the major side-effect order");
}

void DefaultsAndVersionMismatch() {
    auto manager = Manager();
    pal4::BehaviorPathEndpointState helper{};
    Runtime runtime;
    runtime.children[0x8000u] = {0x8200u};
    runtime.integers[{0x8200u, "MSTINFO_Behavior_ObjectType"}] = 0;
    runtime.children[0x8200u] = {0x8400u};
    runtime.names[0x8400u] = "AAB_MST_parameters";
    Accept(manager, helper, runtime);
    Check(runtime.scalars.front() == std::pair{0x7cu, 0.0f} &&
              runtime.scalars[1] == std::pair{0x74u, 0.0f} &&
              runtime.scalars[4] == std::pair{0x68u, 0.0f} &&
              helper.words[0xc0u / 4u] == 1u &&
              (helper.words[0xc4u / 4u] & 0xffffu) == 0u &&
              runtime.animations == std::vector<std::string>{"C02"} &&
              runtime.animation_time == -1.0f,
          "default movement memory and mismatched script version skip time");
}

void LinkableStub() {
    auto manager = Manager();
    auto target = Target();
    pal4::BehaviorPathEndpointState helper{};
    helper.words[47] = 0x1234u;
    pal4::MonsterPathEndpointRestoreRuntimeStub runtime;
    pal4::MonsterPathEndpointRestore::Accept(
        target, manager, helper, 0x6000u, runtime);
    Check(helper.words[0xb8u / 4u] == 0u &&
              helper.words[47] == 0u,
          "default runtime composes path helper while XML and engine services link");
}

void AnimationFlagTargetRestore() {
    auto manager = Manager();
    auto target = Target();
    target.words[0x0cu / 4u] = 0x5000u;
    target.words[0x1cu / 4u] = 0xdead0000u;
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 0xaabbcc00u;
    Runtime runtime;
    runtime.select_x = false;
    runtime.select_c01 = false;
    runtime.integers[{0x9000u, "AAB_MST_memory_last_behavior_id"}] = 42;
    runtime.floats[{0x9000u, "AAB_MST_memory_actiontime"}] = 1.75f;
    pal4::MonsterAnimationFlagRestore::Accept(
        target, manager, helper, 0x6000u, runtime);
    Check(runtime.configured_path == 0u &&
          runtime.configured_parameter == 0x7100u &&
          runtime.scalars ==
              std::vector<std::pair<std::uint32_t, float>>{
                  {0x68u, 0.0f}, {0x5cu, 3.0f}} &&
          runtime.vectors ==
              std::vector<std::pair<std::uint32_t,
                                    pal4::BehaviorVector3>>{
                  {0x14u, {7.0f, 8.0f, 9.0f}},
                  {0x04u, {1.0f, 2.0f, 3.0f}},
                  {0x0cu, {4.0f, 5.0f, 6.0f}},
                  {0x1cu, {10.0f, 11.0f, 12.0f}}} &&
          helper.words[0xb8u / 4u] == 0xaabbcc01u &&
          runtime.grid_element == 0x6000u &&
          runtime.animations == std::vector<std::string>{"X", "C01", "Z01"} &&
          runtime.animation_flag == 1u &&
          runtime.animation_time == 1.75f,
          "class-two target restores transform, grid and time with fallback");
    auto position = [&](std::string_view call) {
        for (std::size_t i = 0; i < runtime.calls.size(); ++i)
            if (runtime.calls[i] == call) return i;
        return runtime.calls.size();
    };
    Check(position("configure-path") < position("scalar") &&
          position("scalar") < position("transform") &&
          position("transform") < position("add-grid") &&
          position("add-grid") < position("select") &&
          position("select") < position("flag") &&
          position("flag") < position("version") &&
          position("version") < position("time"),
          "class-two target side-effect order");

    Runtime first_action;
    pal4::MonsterAnimationFlagRestore::Accept(
        target, manager, helper, 0x6000u, first_action);
    Check(first_action.animations == std::vector<std::string>{"X"} &&
          first_action.animation_time == -1.0f,
          "X success stops fallback and mismatched version skips time");

    Runtime second_action;
    second_action.select_x = false;
    pal4::MonsterAnimationFlagRestore::Accept(
        target, manager, helper, 0x6000u, second_action);
    Check(second_action.animations ==
              std::vector<std::string>{"X", "C01"},
          "C01 success stops Z01 fallback");

    pal4::MonsterPathEndpointRestoreRuntimeStub stub;
    pal4::MonsterAnimationFlagRestore::Accept(
        target, manager, helper, 0x6000u, stub);
}

void AnimationFlagCompletion() {
    auto target = Target();
    target.words[0x0cu / 4u] = 0x5000u;
    target.words[0x1cu / 4u] = 0xdead0000u;
    Runtime runtime;
    Check(!pal4::MonsterAnimationFlagRestore::IsDone(target, runtime),
          "zero flag is incomplete");
    runtime.completion_flag = 0x80000000u;
    Check(pal4::MonsterAnimationFlagRestore::IsDone(target, runtime) &&
          runtime.calls == std::vector<std::string>{
              "completion-flag", "completion-flag"},
          "nonzero flag completes through manager +0x0c");
}

}  // namespace

int main() {
    NoParametersStillConfiguresPath();
    FullRestoreAndAnimationFallback();
    DefaultsAndVersionMismatch();
    LinkableStub();
    AnimationFlagTargetRestore();
    AnimationFlagCompletion();
    std::cout << "monster path endpoint restore cases passed\n";
}
