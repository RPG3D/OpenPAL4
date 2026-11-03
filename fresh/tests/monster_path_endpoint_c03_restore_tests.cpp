#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_path_endpoint_restore.h"

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

using Key = std::pair<std::uint32_t, std::string>;

struct Runtime final : pal4::MonsterPathEndpointRestoreRuntimeStub {
    std::map<std::uint32_t, std::vector<std::uint32_t>> children;
    std::map<std::uint32_t, std::string> names;
    std::map<Key, std::int32_t> ints;
    std::map<Key, float> floats;
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, float>> scalars;
    std::vector<std::pair<std::uint32_t, pal4::BehaviorVector3>> vectors;
    std::vector<std::string> animations;
    std::uint32_t path{0xffffffffu};
    std::uint32_t grid_argument{};
    std::uint32_t inserted{};
    std::uint32_t animation_address{};
    float time{-1.0f};
    bool c03_succeeds{true};
    std::int32_t version{1};

    std::string ReadStringAttribute(std::uint32_t node,
        std::string_view key, std::string_view fallback) override {
        Check(node == 0x8000u && key == "MSTINFO_Behavior_GroupPathWayID" &&
              fallback == "NOPathWay", "group path lookup");
        calls.emplace_back("path-name");
        return "route";
    }
    std::uint32_t ResolveWorldPath(std::string_view name) override {
        Check(name == "route", "group path name");
        calls.emplace_back("resolve-path");
        return 0xb000u;
    }
    std::uint32_t GridParameter(std::uint32_t grid) override {
        Check(grid == 0x7000u, "grid lookup");
        calls.emplace_back("grid-parameter");
        return 0x7100u;
    }
    void ConfigurePath(pal4::BehaviorPathEndpointState&,
        std::uint32_t helper, std::uint32_t parameter,
        std::uint32_t resolved_path) override {
        Check(helper == 0x6000u, "helper address");
        calls.emplace_back("configure-path");
        grid_argument = parameter;
        path = resolved_path;
    }
    std::int32_t ChildCount(std::uint32_t node) override {
        return static_cast<std::int32_t>(children[node].size());
    }
    std::uint32_t Child(std::uint32_t node, std::uint32_t index) override {
        return children[node].at(index);
    }
    std::string ChildName(std::uint32_t node) override { return names[node]; }
    std::int32_t ReadIntAttribute(std::uint32_t node,
        std::string_view key, std::int32_t fallback) override {
        const auto it = ints.find({node, std::string(key)});
        return it == ints.end() ? fallback : it->second;
    }
    float ReadFloatAttribute(std::uint32_t node,
        std::string_view key, float fallback) override {
        const auto it = floats.find({node, std::string(key)});
        return it == floats.end() ? fallback : it->second;
    }
    std::int32_t ReadMemoryInt(std::uint32_t memory,
        std::string_view key, std::int32_t fallback) override {
        Check(memory == 0x9000u, "memory address");
        return ReadIntAttribute(memory, key, fallback);
    }
    float ReadMemoryFloat(std::uint32_t memory,
        std::string_view key, float fallback) override {
        Check(memory == 0x9000u, "memory address");
        return ReadFloatAttribute(memory, key, fallback);
    }
    void SetHelperScalar(std::uint32_t helper,
        std::uint32_t slot, float value) override {
        Check(helper == 0x6000u, "scalar helper address");
        calls.emplace_back("scalar");
        scalars.emplace_back(slot, value);
    }
    pal4::MonsterTransformRows ActorTransform(std::uint32_t actor) override {
        Check(actor == 0xa000u, "actor address");
        calls.emplace_back("transform");
        return {{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}}};
    }
    void SetHelperVector(std::uint32_t helper, std::uint32_t slot,
        const pal4::BehaviorVector3& value) override {
        Check(helper == 0x6000u, "vector helper address");
        calls.emplace_back("vector");
        vectors.emplace_back(slot, value);
    }
    void AddGridElement(std::uint32_t grid, std::uint32_t helper) override {
        Check(grid == 0x7000u, "grid add address");
        calls.emplace_back("add-grid");
        inserted = helper;
    }
    bool SelectAnimation(std::uint32_t address,
        std::string_view name) override {
        Check(address == 0x5170u, "C03 manager pointer offset");
        calls.emplace_back("select");
        animations.emplace_back(name);
        return name == "C03" ? c03_succeeds : true;
    }
    void SetAnimationFlag(std::uint32_t address,
        std::uint32_t flag) override {
        Check(address == 0x5170u && flag == 1u, "animation flag");
        calls.emplace_back("flag");
        animation_address = address;
    }
    std::int32_t ScriptVersion() override {
        calls.emplace_back("version");
        return version;
    }
    void SetAnimationTime(std::uint32_t address, float value) override {
        Check(address == 0x5170u, "animation time address");
        calls.emplace_back("time");
        time = value;
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

void Accept(pal4::MonsterBehaviorManagerState& manager,
            pal4::BehaviorPathEndpointState& helper, Runtime& runtime) {
    pal4::MonsterPathEndpointTargetState target{};
    target.words[0x14u / 4u] = 0x5000u;
    target.words[0x1cu / 4u] = 0xdead0000u;
    pal4::MonsterPathEndpointC03Restore::Accept(
        target, manager, helper, 0x6000u, runtime);
}

void FullRestore() {
    auto manager = Manager();
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xc4u / 4u] = 0x11220000u;
    helper.words[0xb8u / 4u] = 0xaabbcc00u;
    Runtime runtime;
    runtime.c03_succeeds = false;
    runtime.children[0x8000u] = {0x8100u, 0x8200u};
    runtime.ints[{0x8100u, "MSTINFO_Behavior_ObjectType"}] = 0;
    runtime.ints[{0x8200u, "MSTINFO_Behavior_ObjectType"}] = 1;
    runtime.children[0x8200u] = {0x8300u, 0x8400u};
    runtime.names[0x8300u] = "other";
    runtime.names[0x8400u] = "AAB_MST_parameters";
    runtime.floats[{0x8400u, "AABPedestrian_MST_run_max_speed"}] = 12.5f;
    runtime.floats[{0x8400u, "AABPedestrian_MST_run_max_force"}] = 7.0f;
    runtime.ints[{0x8400u, "AABPedestrian_MST_walk_pathfollow"}] = 2;
    runtime.floats[{0x9000u, "AAB_MST_memory_speed"}] = 3.5f;
    runtime.ints[{0x9000u, "AABPedestrian_MST_memory_pathDirection"}] = -1;
    runtime.ints[{0x9000u, "AABPedestrian_MST_memory_wander"}] = 1;
    runtime.ints[{0x9000u, "AAB_MST_memory_last_behavior_id"}] = 1;
    runtime.floats[{0x9000u, "AAB_MST_memory_actiontime"}] = 1.75f;
    Accept(manager, helper, runtime);

    Check(runtime.path == 0xb000u && runtime.grid_argument == 0x7100u &&
          runtime.scalars == std::vector<std::pair<std::uint32_t, float>>{
              {0x7cu, 12.5f}, {0x74u, 7.0f}, {0x68u, 12.5f},
              {0x5cu, 3.0f}, {0x68u, 3.5f}} &&
          helper.words[0xc0u / 4u] == 0xffffffffu &&
          helper.words[0xc4u / 4u] == 0x11220101u &&
          helper.words[0xb8u / 4u] == 0xaabbcc01u &&
          runtime.vectors == std::vector<std::pair<std::uint32_t,
              pal4::BehaviorVector3>>{
                  {0x14u, {7, 8, 9}}, {0x04u, {1, 2, 3}},
                  {0x0cu, {4, 5, 6}}, {0x1cu, {10, 11, 12}}} &&
          runtime.inserted == 0x6000u &&
          runtime.animations == std::vector<std::string>{"C03", "Z02"} &&
          runtime.animation_address == 0x5170u && runtime.time == 1.75f,
          "C03 full restore");
    auto position = [&](std::string_view name) {
        for (std::size_t i = 0; i < runtime.calls.size(); ++i)
            if (runtime.calls[i] == name) return i;
        return runtime.calls.size();
    };
    Check(position("configure-path") < position("scalar") &&
          position("scalar") < position("transform") &&
          position("transform") < position("vector") &&
          position("vector") < position("add-grid") &&
          position("add-grid") < position("select") &&
          position("flag") < position("version") &&
          position("version") < position("time"), "C03 side effect order");
}

void DefaultsAndMissingParameters() {
    auto manager = Manager();
    pal4::BehaviorPathEndpointState helper{};
    Runtime runtime;
    runtime.children[0x8000u] = {0x8100u};
    runtime.ints[{0x8100u, "MSTINFO_Behavior_ObjectType"}] = 0;
    Accept(manager, helper, runtime);
    Check(runtime.path == 0xb000u && runtime.scalars.empty() &&
          runtime.animations.empty(), "wrong object type stops after path setup");

    runtime.children[0x8000u] = {0x8200u};
    runtime.ints[{0x8200u, "MSTINFO_Behavior_ObjectType"}] = 1;
    runtime.children[0x8200u] = {0x8400u};
    runtime.names[0x8400u] = "AAB_MST_parameters";
    Accept(manager, helper, runtime);
    Check(runtime.scalars == std::vector<std::pair<std::uint32_t, float>>{
              {0x7cu, 0.0f}, {0x74u, 0.0f}, {0x68u, 0.0f},
              {0x5cu, 3.0f}, {0x68u, 0.0f}} &&
          helper.words[0xc0u / 4u] == 1u &&
          (helper.words[0xc4u / 4u] & 0xffffu) == 0u &&
          runtime.animations == std::vector<std::string>{"C03"} &&
          runtime.time == -1.0f, "C03 defaults and version mismatch");
}

void LinkableStub() {
    auto manager = Manager();
    pal4::MonsterPathEndpointTargetState target{};
    pal4::BehaviorPathEndpointState helper{};
    helper.words[47] = 0x1234u;
    pal4::MonsterPathEndpointRestoreRuntimeStub runtime;
    pal4::MonsterPathEndpointC03Restore::Accept(
        target, manager, helper, 0x6000u, runtime);
    Check(helper.words[47] == 0u, "C03 default path callback links");
}

}  // namespace

int main() {
    FullRestore();
    DefaultsAndMissingParameters();
    LinkableStub();
    std::cout << "monster path endpoint C03 restore cases passed\n";
}
