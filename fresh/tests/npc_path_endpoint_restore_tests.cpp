#include "pal4/npc_path_endpoint_restore.h"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
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

using NodeKey = std::pair<std::uint32_t, std::string>;

struct Runtime final : pal4::NpcPathEndpointRestoreRuntimeStub {
    std::map<std::uint32_t, std::vector<std::uint32_t>> children;
    std::map<std::uint32_t, std::string> names;
    std::map<NodeKey, std::int32_t> integers;
    std::map<NodeKey, float> floats;
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, float>> scalars;
    std::vector<std::pair<std::uint32_t, pal4::BehaviorVector3>> vectors;
    std::vector<std::string> animations;
    std::uint32_t configured_path{0xffffffffu};
    std::uint32_t configured_parameter{};
    std::uint32_t grid_element{};
    std::uint32_t animation_flag{};
    float animation_time{-1.0f};
    bool select_c02{true};
    bool select_c03{true};

    std::string ReadStringAttribute(std::uint32_t node,
        std::string_view key, std::string_view fallback) override {
        Check(node == 0x8000u &&
              key == "NPCINFO_Behavior_GroupPathWayID" &&
              fallback == "NOPathWay", "NPC path name and fallback");
        calls.emplace_back("read-path");
        return "ElmPath";
    }
    std::uint32_t ResolveWorldPath(std::string_view name) override {
        Check(name == "ElmPath", "path is resolved as a string");
        calls.emplace_back("resolve-path");
        return 0xb000u;
    }
    std::uint32_t GridParameter(std::uint32_t grid) override {
        Check(grid == 0x7000u, "grid parameter source");
        calls.emplace_back("grid-parameter");
        return 0x7100u;
    }
    void ConfigurePath(pal4::BehaviorPathEndpointState&,
        std::uint32_t helper, std::uint32_t grid_parameter,
        std::uint32_t path) override {
        Check(helper == 0x6000u, "helper path address");
        calls.emplace_back("configure-path");
        configured_parameter = grid_parameter;
        configured_path = path;
    }
    std::int32_t ChildCount(std::uint32_t node) override {
        calls.push_back("count:" + std::to_string(node));
        return static_cast<std::int32_t>(children[node].size());
    }
    std::uint32_t Child(std::uint32_t node,
                        std::uint32_t index) override {
        calls.push_back("child:" + std::to_string(node) + ":" +
                        std::to_string(index));
        return children.at(node).at(index);
    }
    std::string ChildName(std::uint32_t node) override {
        calls.emplace_back("child-name");
        return names[node];
    }
    std::int32_t ReadIntAttribute(std::uint32_t node,
        std::string_view key, std::int32_t fallback) override {
        calls.push_back("int:" + std::string(key));
        const auto found = integers.find({node, std::string(key)});
        return found == integers.end() ? fallback : found->second;
    }
    float ReadFloatAttribute(std::uint32_t node,
        std::string_view key, float fallback) override {
        calls.push_back("float:" + std::string(key));
        const auto found = floats.find({node, std::string(key)});
        return found == floats.end() ? fallback : found->second;
    }
    std::int32_t ReadMemoryInt(std::uint32_t memory,
        std::string_view key, std::int32_t fallback) override {
        Check(memory == 0x9000u, "NPC memory address");
        calls.push_back("memory-int:" + std::string(key));
        const auto found = integers.find({memory, std::string(key)});
        return found == integers.end() ? fallback : found->second;
    }
    float ReadMemoryFloat(std::uint32_t memory,
        std::string_view key, float fallback) override {
        Check(memory == 0x9000u, "NPC memory address");
        calls.push_back("memory-float:" + std::string(key));
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
        Check(grid == 0x7000u && helper == 0x6000u,
              "grid insertion arguments");
        calls.emplace_back("add-grid");
        grid_element = helper;
    }
    bool SelectAnimation(std::uint32_t animation,
                         std::string_view name) override {
        Check(animation == 0x5170u, "animation address");
        calls.emplace_back("select-animation");
        animations.emplace_back(name);
        if (name == "C02") return select_c02;
        if (name == "C03") return select_c03;
        return true;
    }
    void SetAnimationFlag(std::uint32_t animation,
                          std::uint32_t flag) override {
        Check(animation == 0x5170u, "animation flag address");
        calls.emplace_back("animation-flag");
        animation_flag = flag;
    }
    std::uint32_t SetAnimationTimeAndReturn(
        std::uint32_t animation, float time) override {
        Check(animation == 0x5170u, "animation time address");
        calls.emplace_back("animation-time");
        animation_time = time;
        return 0x1234u;
    }
};

pal4::GameObjectState Manager() {
    pal4::GameObjectState state{};
    state.words[0x204u / 4u] = 0x7000u;
    state.words[0x6d8u / 4u] = 0x8000u;
    state.words[0x668u / 4u] = 0x9000u;
    state.words[0x14u / 4u] = 0xa000u;
    state.words[0xdcu / 4u] = std::bit_cast<std::uint32_t>(2.0f);
    return state;
}

pal4::NpcPathEndpointTargetState Target() {
    pal4::NpcPathEndpointTargetState target{};
    target.words[0x24u / 4u] = 0x5000u;
    return target;
}

void NoParametersStillConfiguresPath() {
    auto manager = Manager();
    auto target = Target();
    pal4::BehaviorPathEndpointState helper{};
    Runtime runtime;
    runtime.children[0x8000u] = {0x8100u, 0x8200u};
    runtime.integers[{0x8100u, "NPCINFO_Behavior_ObjectType"}] = 1;
    runtime.integers[{0x8200u, "NPCINFO_Behavior_ObjectType"}] = 0;
    runtime.children[0x8200u] = {0x8300u};
    runtime.names[0x8300u] = "Other";
    Check(pal4::NpcPathEndpointRestore::Accept(
              target, manager, helper, 0x6000u, runtime) == 2u &&
          runtime.configured_path == 0xb000u &&
          runtime.configured_parameter == 0x7100u &&
          runtime.scalars.empty() && runtime.vectors.empty() &&
          runtime.animations.empty(),
          "missing parameter child returns outer child count after path setup");
    Check(runtime.calls[0] == "read-path" &&
          runtime.calls[1] == "resolve-path" &&
          runtime.calls[2] == "grid-parameter" &&
          runtime.calls[3] == "configure-path",
          "path setup precedes child scanning");

    runtime.calls.clear();
    manager.words[0x6d8u / 4u] = 0u;
    Check(pal4::NpcPathEndpointRestore::Accept(
              target, manager, helper, 0x6000u, runtime) == 0u &&
          runtime.configured_path == 0u &&
          runtime.calls[0] == "grid-parameter",
          "null behavior config skips pathway lookup");
}

void FullRestoreAndAnimationGate() {
    const auto manager = Manager();
    const auto target = Target();
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xc4u / 4u] = 0xdeadbeefu;
    Runtime runtime;
    runtime.children[0x8000u] = {0x8100u, 0x8200u};
    runtime.integers[{0x8100u, "NPCINFO_Behavior_ObjectType"}] = 1;
    runtime.integers[{0x8200u, "NPCINFO_Behavior_ObjectType"}] = 0;
    runtime.children[0x8200u] = {0x8300u, 0x8400u};
    runtime.names[0x8300u] = "Other";
    runtime.names[0x8400u] = "AAB_NPC_parameters";
    runtime.floats[{0x8400u, "AABPedestrian_NPC_walk_max_speed"}] = 4.0f;
    runtime.floats[{0x8400u, "AABPedestrian_NPC_walk_max_force"}] = 5.0f;
    runtime.integers[{0x8400u, "AABPedestrian_NPC_walk_pathfollow"}] = 2;
    runtime.floats[{0x9000u, "AAB_NPC_memory_speed"}] = 6.0f;
    runtime.integers[{0x9000u,
        "AABPedestrian_NPC_memory_pathDirection"}] = -1;
    runtime.integers[{0x9000u, "AABPedestrian_NPC_memory_wander"}] = 3;
    runtime.integers[{0x9000u, "AAB_NPC_memory_last_behavior_id"}] = 0;
    runtime.floats[{0x9000u, "AAB_NPC_memory_actiontime"}] = 3.5f;
    runtime.select_c02 = false;
    Check(pal4::NpcPathEndpointRestore::Accept(
              target, manager, helper, 0x6000u, runtime) == 0x1234u &&
          runtime.scalars == std::vector<std::pair<std::uint32_t, float>>{
              {0x7cu, 4.0f}, {0x74u, 5.0f}, {0x68u, 4.0f},
              {0x5cu, 2.0f}, {0x68u, 6.0f}} &&
          runtime.vectors == std::vector<
              std::pair<std::uint32_t, pal4::BehaviorVector3>>{
              {0x14u, {7.0f, 8.0f, 9.0f}},
              {0x04u, {1.0f, 2.0f, 3.0f}},
              {0x0cu, {4.0f, 5.0f, 6.0f}},
              {0x1cu, {10.0f, 11.0f, 12.0f}}},
          "NPC movement scalars, transform order and action-time return");
    Check(helper.words[0xc0u / 4u] == 0xffffffffu &&
          (helper.words[0xc4u / 4u] & 0xffffu) == 0x101u &&
          (helper.words[0xc4u / 4u] & 0xffff0000u) == 0xdead0000u &&
          (helper.words[0xb8u / 4u] & 0xffu) == 1u &&
          runtime.grid_element == 0x6000u &&
          runtime.animations == std::vector<std::string>{"C02", "Z02"} &&
          runtime.animation_flag == 1u && runtime.animation_time == 3.5f,
          "NPC direction, flags, grid and animation fallback");

    runtime.calls.clear();
    runtime.animations.clear();
    runtime.animation_time = -1.0f;
    runtime.select_c02 = true;
    runtime.integers[{0x9000u, "AAB_NPC_memory_last_behavior_id"}] = 2;
    Check(pal4::NpcPathEndpointRestore::Accept(
              target, manager, helper, 0x6000u, runtime) == 0u &&
          runtime.animations == std::vector<std::string>{"C02"} &&
          runtime.animation_time == -1.0f,
          "mismatched behavior ID skips action-time restore");

    runtime.integers.erase({0x9000u, "AAB_NPC_memory_last_behavior_id"});
    runtime.calls.clear();
    runtime.animation_time = -1.0f;
    Check(pal4::NpcPathEndpointRestore::Accept(
              target, manager, helper, 0x6000u, runtime) == 0u &&
          runtime.animation_time == -1.0f,
          "missing last-behavior ID uses nonmatching sentinel");
}

void C03UsesRunParametersAndClassOne() {
    const auto manager = Manager();
    auto target = Target();
    target.words[0x1cu / 4u] = 0x5000u;
    target.words[0x24u / 4u] = 0xdeadbeefu;
    pal4::BehaviorPathEndpointState helper{};
    Runtime runtime;
    runtime.children[0x8000u] = {0x8100u, 0x8200u};
    runtime.integers[{0x8100u, "NPCINFO_Behavior_ObjectType"}] = 0;
    runtime.integers[{0x8200u, "NPCINFO_Behavior_ObjectType"}] = 1;
    runtime.children[0x8200u] = {0x8400u};
    runtime.names[0x8400u] = "AAB_NPC_parameters";
    runtime.floats[{0x8400u, "AABPedestrian_NPC_run_max_speed"}] = 8.0f;
    runtime.floats[{0x8400u, "AABPedestrian_NPC_run_max_force"}] = 9.0f;
    runtime.integers[{0x8400u, "AABPedestrian_NPC_walk_pathfollow"}] = 1;
    runtime.integers[{0x9000u, "AAB_NPC_memory_last_behavior_id"}] = 1;
    runtime.floats[{0x9000u, "AAB_NPC_memory_actiontime"}] = 4.5f;
    runtime.select_c03 = false;
    Check(pal4::NpcPathEndpointC03Restore::Accept(
              target, manager, helper, 0x6000u, runtime) == 0x1234u &&
          runtime.scalars == std::vector<std::pair<std::uint32_t, float>>{
              {0x7cu, 8.0f}, {0x74u, 9.0f}, {0x68u, 8.0f},
              {0x5cu, 3.0f}, {0x68u, 0.0f}} &&
          runtime.animations == std::vector<std::string>{"C03", "Z02"} &&
          runtime.animation_time == 4.5f &&
          (helper.words[0xc4u / 4u] & 0xffffu) == 0x100u,
          "C03 uses class one, run keys, scaled property and fallback");

    Runtime no_class_one;
    no_class_one.children[0x8000u] = {0x8100u};
    no_class_one.integers[{0x8100u, "NPCINFO_Behavior_ObjectType"}] = 0;
    pal4::BehaviorPathEndpointState empty_helper{};
    Check(pal4::NpcPathEndpointC03Restore::Accept(
              target, manager, empty_helper, 0x6000u, no_class_one) == 1u &&
          no_class_one.scalars.empty(),
          "C03 ignores class-zero target nodes");
}

}  // namespace

int main() {
    NoParametersStillConfiguresPath();
    FullRestoreAndAnimationGate();
    C03UsesRunParametersAndClassOne();
}
