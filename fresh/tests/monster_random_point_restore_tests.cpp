#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_random_point_restore.h"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
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

struct Runtime final : pal4::MonsterPathEndpointRestoreRuntimeStub,
                       pal4::IMonsterRandomPointRuntime {
    std::map<std::uint32_t, std::vector<std::uint32_t>> children;
    std::map<std::uint32_t, std::string> names;
    std::map<Key, std::int32_t> ints;
    std::map<Key, float> floats;
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, float>> scalars;
    std::vector<std::pair<std::uint32_t, pal4::BehaviorVector3>> vectors;
    std::vector<std::string> animations;
    std::uint32_t path{0xffffffffu};
    std::uint32_t grid_parameter{};
    std::uint32_t inserted{};
    std::uint32_t flag{};
    float time{-1.0f};
    bool c02_succeeds{true};
    std::uint32_t point_reads{};
    std::uint32_t type_reads{};

    std::string ReadStringAttribute(std::uint32_t node,
        std::string_view key, std::string_view fallback) override {
        Check(node == 0x8000u && key == "MSTINFO_Behavior_GroupPathWayID" &&
              fallback == "NOPathWay", "path attribute");
        calls.emplace_back("path-name");
        return "wander-route";
    }
    std::uint32_t ResolveWorldPath(std::string_view name) override {
        Check(name == "wander-route", "route name");
        calls.emplace_back("resolve-path");
        return 0xb000u;
    }
    std::uint32_t GridParameter(std::uint32_t grid) override {
        Check(grid == 0x7000u, "grid parameter address");
        calls.emplace_back("grid-parameter");
        return 0x7100u;
    }
    void ConfigurePath(pal4::BehaviorPathEndpointState&,
        std::uint32_t helper, std::uint32_t parameter,
        std::uint32_t resolved_path) override {
        Check(helper == 0x6000u, "configure helper address");
        calls.emplace_back("configure-path");
        grid_parameter = parameter;
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
        calls.emplace_back("parameter:" + std::string(key));
        const auto it = floats.find({node, std::string(key)});
        return it == floats.end() ? fallback : it->second;
    }
    std::int32_t ReadMemoryInt(std::uint32_t memory,
        std::string_view key, std::int32_t fallback) override {
        Check(memory == 0x9000u, "memory int address");
        calls.emplace_back("memory:" + std::string(key));
        const auto it = ints.find({memory, std::string(key)});
        return it == ints.end() ? fallback : it->second;
    }
    float ReadMemoryFloat(std::uint32_t memory,
        std::string_view key, float fallback) override {
        Check(memory == 0x9000u, "memory float address");
        calls.emplace_back("memory:" + std::string(key));
        const auto it = floats.find({memory, std::string(key)});
        return it == floats.end() ? fallback : it->second;
    }
    void SetHelperScalar(std::uint32_t helper,
        std::uint32_t slot, float value) override {
        Check(helper == 0x6000u, "scalar helper address");
        calls.emplace_back("scalar");
        scalars.emplace_back(slot, value);
    }
    pal4::MonsterTransformRows ActorTransform(
        std::uint32_t actor) override {
        Check(actor == 0xa000u, "actor transform address");
        calls.emplace_back("transform");
        return {{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}}};
    }
    void SetHelperVector(std::uint32_t helper, std::uint32_t slot,
        const pal4::BehaviorVector3& value) override {
        Check(helper == 0x6000u, "vector helper address");
        calls.emplace_back("vector");
        vectors.emplace_back(slot, value);
    }
    void AddGridElement(std::uint32_t grid,
        std::uint32_t helper) override {
        Check(grid == 0x7000u, "add grid address");
        calls.emplace_back("add-grid");
        inserted = helper;
    }
    bool SelectAnimation(std::uint32_t address,
        std::string_view name) override {
        Check(address == 0x5170u, "animation manager offset +4");
        calls.emplace_back("select");
        animations.emplace_back(name);
        return name == "C02" ? c02_succeeds : true;
    }
    void SetAnimationFlag(std::uint32_t address,
        std::uint32_t value) override {
        Check(address == 0x5170u, "animation flag address");
        calls.emplace_back("flag");
        flag = value;
    }
    void SetAnimationTime(std::uint32_t address, float value) override {
        Check(address == 0x5170u, "animation time address");
        calls.emplace_back("time");
        time = value;
    }
    std::int32_t SoundRuntimeType() override {
        calls.emplace_back("sound-type");
        ++type_reads;
        return 3;
    }
    pal4::BehaviorVector3 CurrentHelperPoint(
        std::uint32_t helper) override {
        Check(helper == 0x6000u, "current point helper address");
        calls.emplace_back("current-point");
        ++point_reads;
        return {4.0f, 5.0f, 6.0f};
    }
    float ReadSoundActionTime(std::uint32_t memory,
        float fallback) override {
        Check(memory == 0x9000u && fallback == 0.0f,
              "sound action time lookup");
        calls.emplace_back("sound-time");
        const auto it = floats.find({memory, "AAB_MST_memory_actiontime"});
        return it == floats.end() ? fallback : it->second;
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
    target.words[0x04u / 4u] = 0x5000u;
    target.words[0x1cu / 4u] = 0xdead0000u;
    return target;
}

void Accept(pal4::BehaviorPathEndpointState& helper, Runtime& runtime) {
    const auto target = Target();
    const auto manager = Manager();
    pal4::MonsterRandomPointRestore::Accept(
        target, manager, helper, 0x6000u, runtime, runtime);
}

void MissingParameters() {
    pal4::BehaviorPathEndpointState helper{};
    Runtime runtime;
    runtime.children[0x8000u] = {0x8100u};
    runtime.ints[{0x8100u, "MSTINFO_Behavior_ObjectType"}] = 1;
    Accept(helper, runtime);
    Check(runtime.path == 0xb000u && runtime.grid_parameter == 0x7100u &&
          runtime.scalars.empty() && runtime.inserted == 0u &&
          runtime.calls == std::vector<std::string>{
              "path-name", "resolve-path", "grid-parameter",
              "configure-path"},
          "wrong object type stops after path configuration");
}

void ConfigureParameters(Runtime& runtime) {
    runtime.children[0x8000u] = {0x8100u, 0x8200u};
    runtime.ints[{0x8100u, "MSTINFO_Behavior_ObjectType"}] = 2;
    runtime.ints[{0x8200u, "MSTINFO_Behavior_ObjectType"}] = 3;
    runtime.children[0x8200u] = {0x8300u, 0x8400u};
    runtime.names[0x8300u] = "other";
    runtime.names[0x8400u] = "AAB_MST_parameters";
}

void SavedSoundPoint() {
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 0xaabbcc00u;
    Runtime runtime;
    ConfigureParameters(runtime);
    runtime.c02_succeeds = false;
    runtime.floats[{0x8400u, "AABWander_MST_max_speed"}] = 12.5f;
    runtime.floats[{0x8400u, "AABWander_MST_max_force"}] = 7.0f;
    runtime.floats[{0x8400u, "AABWander_MST_max_range"}] = 30.0f;
    runtime.ints[{0x9000u, "AAB_MST_memory_last_behavior_id"}] = 3;
    runtime.floats[{0x9000u, "AAB_MST_memory_speed"}] = 3.5f;
    runtime.floats[{0x9000u, "AABWander_MST_memory_start_x"}] = 10.0f;
    runtime.floats[{0x9000u, "AABWander_MST_memory_start_y"}] = 20.0f;
    runtime.floats[{0x9000u, "AABWander_MST_memory_start_z"}] = 30.0f;
    runtime.floats[{0x9000u, "AAB_MST_memory_actiontime"}] = 1.75f;
    Accept(helper, runtime);
    Check(runtime.scalars ==
              std::vector<std::pair<std::uint32_t, float>>{
                  {0x7cu, 12.5f}, {0x74u, 7.0f}, {0x68u, 12.5f},
                  {0x5cu, 3.0f}, {0x68u, 3.5f}} &&
          runtime.vectors ==
              std::vector<std::pair<std::uint32_t,
                                    pal4::BehaviorVector3>>{
                  {0x14u, {7, 8, 9}}, {0x04u, {1, 2, 3}},
                  {0x0cu, {4, 5, 6}}, {0x1cu, {10, 11, 12}}} &&
          helper.words[0xd4u / 4u] == std::bit_cast<std::uint32_t>(10.0f) &&
          helper.words[0xd8u / 4u] == std::bit_cast<std::uint32_t>(20.0f) &&
          helper.words[0xdcu / 4u] == std::bit_cast<std::uint32_t>(30.0f) &&
          helper.words[0xbcu / 4u] == std::bit_cast<std::uint32_t>(30.0f) &&
          helper.words[0xb8u / 4u] == 0xaabbcc01u &&
          runtime.inserted == 0x6000u && runtime.flag == 1u &&
          runtime.animations == std::vector<std::string>{"C02", "Z02"} &&
          runtime.time == 1.75f && runtime.point_reads == 0u &&
          runtime.type_reads == 2u,
          "sound target restores saved point, range and animation time");
    auto position = [&](std::string_view call) {
        for (std::size_t i = 0; i < runtime.calls.size(); ++i)
            if (runtime.calls[i] == call) return i;
        return runtime.calls.size();
    };
    Check(position("configure-path") < position("scalar") &&
          position("scalar") < position("memory:AAB_MST_memory_last_behavior_id") &&
          position("memory:AAB_MST_memory_speed") < position("transform") &&
          position("transform") < position("sound-type") &&
          position("sound-type") < position("add-grid") &&
          position("add-grid") < position("select") &&
          position("flag") < position("sound-time") &&
          position("sound-time") < position("time"),
          "sound target preserves major side-effect order");
}

void InvalidSoundPoint() {
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xd4u / 4u] = std::bit_cast<std::uint32_t>(11.0f);
    helper.words[0xd8u / 4u] = std::bit_cast<std::uint32_t>(12.0f);
    helper.words[0xdcu / 4u] = std::bit_cast<std::uint32_t>(13.0f);
    Runtime runtime;
    ConfigureParameters(runtime);
    runtime.ints[{0x9000u, "AAB_MST_memory_last_behavior_id"}] = 3;
    runtime.floats[{0x9000u, "AABWander_MST_memory_start_x"}] = 1.0f;
    runtime.floats[{0x9000u, "AABWander_MST_memory_start_y"}] =
        std::numeric_limits<float>::quiet_NaN();
    runtime.floats[{0x9000u, "AABWander_MST_memory_start_z"}] = 3.0f;
    Accept(helper, runtime);
    Check(helper.words[0xd4u / 4u] == std::bit_cast<std::uint32_t>(11.0f) &&
          helper.words[0xd8u / 4u] == std::bit_cast<std::uint32_t>(12.0f) &&
          helper.words[0xdcu / 4u] == std::bit_cast<std::uint32_t>(13.0f) &&
          runtime.time == 0.0f && runtime.point_reads == 0u,
          "invalid sound coordinates preserve prior point but restore time");
}

void CurrentPointAndDefaults() {
    pal4::BehaviorPathEndpointState helper{};
    Runtime runtime;
    ConfigureParameters(runtime);
    runtime.ints[{0x9000u, "AAB_MST_memory_last_behavior_id"}] = 2;
    Accept(helper, runtime);
    Check(runtime.scalars ==
              std::vector<std::pair<std::uint32_t, float>>{
                  {0x7cu, 0.0f}, {0x74u, 0.0f}, {0x68u, 0.0f},
                  {0x5cu, 3.0f}, {0x68u, 0.0f}} &&
          helper.words[0xd4u / 4u] == std::bit_cast<std::uint32_t>(4.0f) &&
          helper.words[0xd8u / 4u] == std::bit_cast<std::uint32_t>(5.0f) &&
          helper.words[0xdcu / 4u] == std::bit_cast<std::uint32_t>(6.0f) &&
          helper.words[0xbcu / 4u] == 0u &&
          runtime.animations == std::vector<std::string>{"C02"} &&
          runtime.time == -1.0f && runtime.point_reads == 1u,
          "non-sound target uses current helper point and zero defaults");
}

void LinkableStubs() {
    auto manager = Manager();
    auto target = Target();
    pal4::BehaviorPathEndpointState helper{};
    helper.words[47] = 0x1234u;
    pal4::MonsterPathEndpointRestoreRuntimeStub runtime;
    pal4::MonsterRandomPointRuntimeStub point_runtime;
    pal4::MonsterRandomPointRestore::Accept(
        target, manager, helper, 0x6000u, runtime, point_runtime);
    Check(helper.words[47] == 0u, "default path helper links");
}

}  // namespace

int main() {
    MissingParameters();
    SavedSoundPoint();
    InvalidSoundPoint();
    CurrentPointAndDefaults();
    LinkableStubs();
    std::cout << "monster random point restore cases passed\n";
}
