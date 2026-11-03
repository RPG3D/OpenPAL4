#include "pal4/npc_random_point_restore.h"

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

struct Runtime final : pal4::NpcPathEndpointRestoreRuntimeStub,
                       pal4::IMonsterRandomPointRuntime {
    std::map<std::uint32_t, std::vector<std::uint32_t>> children;
    std::map<std::uint32_t, std::string> names;
    std::map<Key, std::int32_t> integers;
    std::map<Key, float> floats;
    std::vector<std::string> calls;
    std::vector<std::string> animations;
    std::vector<std::pair<std::uint32_t, float>> scalars;
    std::vector<std::pair<std::uint32_t, pal4::BehaviorVector3>> vectors;
    std::uint32_t configured_path{0xffffffffu};
    std::uint32_t grid_parameter{};
    bool c02_succeeds{true};
    float restored_time{-1.0f};
    std::uint32_t type_reads{};
    std::uint32_t current_point_reads{};

    std::uint32_t GridParameter(std::uint32_t grid) override {
        Check(grid == 0x7000u, "grid pointer");
        calls.emplace_back("grid-parameter");
        return 0x7020u;
    }
    void ConfigurePath(pal4::BehaviorPathEndpointState&,
                       std::uint32_t helper, std::uint32_t parameter,
                       std::uint32_t path) override {
        Check(helper == 0x6000u, "path helper pointer");
        calls.emplace_back("configure-path");
        grid_parameter = parameter;
        configured_path = path;
    }
    std::int32_t ChildCount(std::uint32_t node) override {
        calls.emplace_back("count");
        return static_cast<std::int32_t>(children[node].size());
    }
    std::uint32_t Child(std::uint32_t node, std::uint32_t index) override {
        calls.emplace_back("child");
        return children[node].at(index);
    }
    std::string ChildName(std::uint32_t node) override {
        calls.emplace_back("child-name");
        return names[node];
    }
    std::int32_t ReadIntAttribute(std::uint32_t node,
                                  std::string_view key,
                                  std::int32_t fallback) override {
        Check(key == "NPCINFO_Behavior_ObjectType" && fallback == -1,
              "object type key and default");
        calls.emplace_back("object-type");
        const auto it = integers.find({node, std::string(key)});
        return it == integers.end() ? fallback : it->second;
    }
    float ReadFloatAttribute(std::uint32_t node,
                             std::string_view key, float fallback) override {
        Check(node == 0x8400u && fallback == 0.0f,
              "wander parameter node and default");
        calls.emplace_back("parameter:" + std::string(key));
        const auto it = floats.find({node, std::string(key)});
        return it == floats.end() ? fallback : it->second;
    }
    std::int32_t ReadMemoryInt(std::uint32_t memory,
                               std::string_view key,
                               std::int32_t fallback) override {
        Check(memory == 0x9000u &&
              key == "AAB_NPC_memory_last_behavior_id" &&
              fallback == 0x7fffffff, "behavior ID lookup");
        calls.emplace_back("behavior-id");
        const auto it = integers.find({memory, std::string(key)});
        return it == integers.end() ? fallback : it->second;
    }
    float ReadMemoryFloat(std::uint32_t memory,
                          std::string_view key, float fallback) override {
        Check(memory == 0x9000u, "NPC memory pointer");
        calls.emplace_back("memory:" + std::string(key));
        const auto it = floats.find({memory, std::string(key)});
        return it == floats.end() ? fallback : it->second;
    }
    void SetHelperScalar(std::uint32_t helper,
                         std::uint32_t slot, float value) override {
        Check(helper == 0x6000u, "scalar helper pointer");
        calls.emplace_back("scalar");
        scalars.emplace_back(slot, value);
    }
    pal4::MonsterTransformRows ActorTransform(std::uint32_t actor) override {
        Check(actor == 0xa000u, "actor transform pointer");
        calls.emplace_back("transform");
        return {{{1, 2, 3}, {4, 5, 6},
                 {7, 8, 9}, {10, 11, 12}}};
    }
    void SetHelperVector(std::uint32_t helper, std::uint32_t slot,
                         const pal4::BehaviorVector3& value) override {
        Check(helper == 0x6000u, "vector helper pointer");
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
        calls.emplace_back("animation");
        animations.emplace_back(name);
        return name == "C02" ? c02_succeeds : true;
    }
    void SetAnimationFlag(std::uint32_t animation,
                          std::uint32_t flag) override {
        Check(animation == 0x5170u && flag == 1u,
              "animation active flag");
        calls.emplace_back("animation-flag");
    }
    std::uint32_t SetAnimationTimeAndReturn(
        std::uint32_t animation, float time) override {
        Check(animation == 0x5170u, "animation time address");
        calls.emplace_back("animation-time");
        restored_time = time;
        return 0x1234u;
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
        ++current_point_reads;
        return {4.0f, 5.0f, 6.0f};
    }
    float ReadSoundActionTime(std::uint32_t, float) override {
        Check(false, "NPC action time uses the NPC memory key");
        return 0.0f;
    }
};

pal4::GameObjectState Manager() {
    pal4::GameObjectState manager{};
    manager.words[0x204u / 4u] = 0x7000u;
    manager.words[0x6d8u / 4u] = 0x8000u;
    manager.words[0x668u / 4u] = 0x9000u;
    manager.words[0x14u / 4u] = 0xa000u;
    manager.words[0xdcu / 4u] = std::bit_cast<std::uint32_t>(2.0f);
    return manager;
}

pal4::NpcPathEndpointTargetState Target() {
    pal4::NpcPathEndpointTargetState target{};
    target.words[0x0cu / 4u] = 0x5000u;
    return target;
}

void Configure(Runtime& runtime) {
    runtime.children[0x8000u] = {0x8100u, 0x8200u};
    runtime.integers[{0x8100u, "NPCINFO_Behavior_ObjectType"}] = 1;
    runtime.integers[{0x8200u, "NPCINFO_Behavior_ObjectType"}] = 3;
    runtime.children[0x8200u] = {0x8300u, 0x8400u};
    runtime.names[0x8300u] = "other";
    runtime.names[0x8400u] = "AAB_NPC_parameters";
}

std::uint32_t Accept(pal4::BehaviorPathEndpointState& helper,
                     Runtime& runtime) {
    return pal4::NpcRandomPointRestore::Accept(
        Target(), Manager(), helper, 0x6000u, runtime, runtime);
}

void MissingParameters() {
    pal4::BehaviorPathEndpointState helper{};
    Runtime runtime;
    runtime.children[0x8000u] = {0x8100u};
    runtime.integers[{0x8100u, "NPCINFO_Behavior_ObjectType"}] = 1;
    Check(Accept(helper, runtime) == 1u &&
          runtime.grid_parameter == 0x7020u &&
          runtime.configured_path == 0u && runtime.scalars.empty() &&
          runtime.calls[0] == "grid-parameter" &&
          runtime.calls[1] == "configure-path",
          "missing type-three parameters return child count after setup");
}

void SavedPoint() {
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 0xaabbcc00u;
    Runtime runtime;
    Configure(runtime);
    runtime.c02_succeeds = false;
    runtime.floats[{0x8400u, "AABWander_NPC_max_speed"}] = 12.5f;
    runtime.floats[{0x8400u, "AABWander_NPC_max_force"}] = 7.0f;
    runtime.floats[{0x8400u, "AABWander_NPC_max_range"}] = 30.0f;
    runtime.integers[{0x9000u, "AAB_NPC_memory_last_behavior_id"}] = 3;
    runtime.floats[{0x9000u, "AAB_NPC_memory_speed"}] = 3.5f;
    runtime.floats[{0x9000u, "AABWander_NPC_memory_start_x"}] = 10.0f;
    runtime.floats[{0x9000u, "AABWander_NPC_memory_start_y"}] = 20.0f;
    runtime.floats[{0x9000u, "AABWander_NPC_memory_start_z"}] = 30.0f;
    runtime.floats[{0x9000u, "AAB_NPC_memory_actiontime"}] = 1.75f;
    Check(Accept(helper, runtime) == 0x1234u &&
          runtime.scalars ==
              std::vector<std::pair<std::uint32_t, float>>{
                  {0x7cu, 12.5f}, {0x74u, 7.0f}, {0x68u, 12.5f},
                  {0x5cu, 3.0f}, {0x68u, 3.5f}} &&
          runtime.vectors ==
              std::vector<std::pair<std::uint32_t,
                                    pal4::BehaviorVector3>>{
                  {0x14u, {7, 8, 9}}, {0x04u, {1, 2, 3}},
                  {0x0cu, {4, 5, 6}}, {0x1cu, {10, 11, 12}}},
          "NPC wander parameters and transform order");
    Check(helper.words[0xd4u / 4u] == std::bit_cast<std::uint32_t>(10.0f) &&
          helper.words[0xd8u / 4u] == std::bit_cast<std::uint32_t>(20.0f) &&
          helper.words[0xdcu / 4u] == std::bit_cast<std::uint32_t>(30.0f) &&
          helper.words[0xbcu / 4u] == std::bit_cast<std::uint32_t>(30.0f) &&
          helper.words[0xb8u / 4u] == 0xaabbcc01u &&
          runtime.animations == std::vector<std::string>{"C02", "Z02"} &&
          runtime.restored_time == 1.75f && runtime.type_reads == 2u &&
          runtime.current_point_reads == 0u,
          "saved point, range, animation fallback and action time");
}

void InvalidPointAndMismatch() {
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xd4u / 4u] = std::bit_cast<std::uint32_t>(11.0f);
    helper.words[0xd8u / 4u] = std::bit_cast<std::uint32_t>(12.0f);
    helper.words[0xdcu / 4u] = std::bit_cast<std::uint32_t>(13.0f);
    Runtime runtime;
    Configure(runtime);
    runtime.integers[{0x9000u, "AAB_NPC_memory_last_behavior_id"}] = 3;
    runtime.floats[{0x9000u, "AABWander_NPC_memory_start_x"}] = 1.0f;
    runtime.floats[{0x9000u, "AABWander_NPC_memory_start_y"}] =
        std::numeric_limits<float>::quiet_NaN();
    runtime.floats[{0x9000u, "AABWander_NPC_memory_start_z"}] = 3.0f;
    Check(Accept(helper, runtime) == 0x1234u &&
          helper.words[0xd4u / 4u] == std::bit_cast<std::uint32_t>(11.0f) &&
          helper.words[0xd8u / 4u] == std::bit_cast<std::uint32_t>(12.0f) &&
          helper.words[0xdcu / 4u] == std::bit_cast<std::uint32_t>(13.0f) &&
          runtime.restored_time == 0.0f,
          "invalid saved coordinate preserves helper point but restores time");

    runtime.integers[{0x9000u, "AAB_NPC_memory_last_behavior_id"}] = 1;
    runtime.restored_time = -1.0f;
    Check(Accept(helper, runtime) == 3u &&
          helper.words[0xd4u / 4u] == std::bit_cast<std::uint32_t>(4.0f) &&
          helper.words[0xd8u / 4u] == std::bit_cast<std::uint32_t>(5.0f) &&
          helper.words[0xdcu / 4u] == std::bit_cast<std::uint32_t>(6.0f) &&
          runtime.current_point_reads == 1u &&
          runtime.restored_time == -1.0f,
          "mismatched behavior uses current point and returns type ID");
}

}  // namespace

int main() {
    MissingParameters();
    SavedPoint();
    InvalidPointAndMismatch();
    std::cout << "NPC random point restore cases passed\n";
}
