#include "pal4/npc_curve_follow_target.h"

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
using Vector = pal4::BehaviorVector3;

struct RestoreRuntime final : pal4::NpcPathEndpointRestoreRuntimeStub {
    std::map<std::uint32_t, std::vector<std::uint32_t>> children;
    std::map<std::uint32_t, std::string> names;
    std::map<Key, std::int32_t> integers;
    std::map<Key, float> floats;
    std::vector<std::string> calls;
    std::vector<std::string> animations;
    std::vector<std::pair<std::uint32_t, float>> scalars;
    std::vector<std::pair<std::uint32_t, Vector>> vectors;
    std::uint32_t configured_path{0xffffffffu};
    bool c02_succeeds{};
    float animation_time{-1.0f};

    std::string ReadStringAttribute(std::uint32_t node,
        std::string_view key, std::string_view fallback) override {
        Check(node == 0x8000u &&
              key == "NPCINFO_Behavior_GroupPathWayID" &&
              fallback == "NOPathWay", "NPC pathway lookup");
        calls.emplace_back("path-name");
        return "curve-route";
    }
    std::uint32_t ResolveWorldPath(std::string_view name) override {
        Check(name == "curve-route", "route lookup name");
        calls.emplace_back("resolve-path");
        return 0xb000u;
    }
    std::uint32_t GridParameter(std::uint32_t grid) override {
        Check(grid == 0x7000u, "grid parameter pointer");
        calls.emplace_back("grid-parameter");
        return 0x7020u;
    }
    void ConfigurePath(pal4::BehaviorPathEndpointState&,
                       std::uint32_t helper, std::uint32_t parameter,
                       std::uint32_t path) override {
        Check(helper == 0x6000u && parameter == 0x7020u,
              "path helper and grid parameter");
        calls.emplace_back("configure-path");
        configured_path = path;
    }
    std::int32_t ChildCount(std::uint32_t node) override {
        calls.emplace_back("child-count");
        return static_cast<std::int32_t>(children[node].size());
    }
    std::uint32_t Child(std::uint32_t node,
                        std::uint32_t index) override {
        calls.emplace_back("child");
        return children[node].at(index);
    }
    std::int32_t ReadIntAttribute(std::uint32_t node,
        std::string_view key, std::int32_t fallback) override {
        Check(key == "NPCINFO_Behavior_ObjectType" && fallback == -1,
              "class-four object type lookup");
        calls.emplace_back("object-type");
        const auto it = integers.find({node, std::string(key)});
        return it == integers.end() ? fallback : it->second;
    }
    std::string ChildName(std::uint32_t node) override {
        calls.emplace_back("child-name");
        return names[node];
    }
    float ReadFloatAttribute(std::uint32_t node,
        std::string_view key, float fallback) override {
        Check(node == 0x8400u &&
              key == "AABSeekOnPath_NPC_walk_max_speed" &&
              fallback == 0.0f, "curve speed lookup");
        calls.emplace_back("max-speed");
        const auto it = floats.find({node, std::string(key)});
        return it == floats.end() ? fallback : it->second;
    }
    void SetHelperScalar(std::uint32_t helper,
                         std::uint32_t slot, float value) override {
        Check(helper == 0x6000u, "scalar helper pointer");
        calls.emplace_back("scalar");
        scalars.emplace_back(slot, value);
    }
    float ReadMemoryFloat(std::uint32_t memory,
        std::string_view key, float fallback) override {
        Check(memory == 0x9000u && fallback == 0.0f,
              "NPC memory float pointer and default");
        calls.emplace_back(std::string(key));
        const auto it = floats.find({memory, std::string(key)});
        return it == floats.end() ? fallback : it->second;
    }
    pal4::MonsterTransformRows ActorTransform(
        std::uint32_t actor) override {
        Check(actor == 0xa000u, "actor transform pointer");
        calls.emplace_back("transform");
        return {{{1, 2, 3}, {4, 5, 6},
                 {7, 8, 9}, {10, 11, 12}}};
    }
    void SetHelperVector(std::uint32_t helper,
        std::uint32_t slot, const Vector& vector) override {
        Check(helper == 0x6000u, "vector helper pointer");
        calls.emplace_back("vector");
        vectors.emplace_back(slot, vector);
    }
    void AddGridElement(std::uint32_t grid,
                        std::uint32_t helper) override {
        Check(grid == 0x7000u && helper == 0x6000u,
              "grid insertion pointers");
        calls.emplace_back("add-grid");
    }
    bool SelectAnimation(std::uint32_t animation,
                         std::string_view name) override {
        Check(animation == 0x5170u, "curve animation address");
        calls.emplace_back("select");
        animations.emplace_back(name);
        return name == "C02" ? c02_succeeds : true;
    }
    void SetAnimationFlag(std::uint32_t animation,
                          std::uint32_t flag) override {
        Check(animation == 0x5170u && flag == 1u,
              "curve animation flag");
        calls.emplace_back("flag");
    }
    std::int32_t ReadMemoryInt(std::uint32_t memory,
        std::string_view key, std::int32_t fallback) override {
        Check(memory == 0x9000u &&
              key == "AAB_NPC_memory_last_behavior_id" &&
              fallback == 0x7fffffff, "last behavior lookup");
        calls.emplace_back("last-behavior");
        const auto it = integers.find({memory, std::string(key)});
        return it == integers.end() ? fallback : it->second;
    }
    std::uint32_t SetAnimationTimeAndReturn(
        std::uint32_t animation, float time) override {
        Check(animation == 0x5170u, "curve action time address");
        calls.emplace_back("set-time");
        animation_time = time;
        return 0x1234u;
    }
};

struct SaveRuntime final : pal4::MonsterPathEndpointSaveRuntimeStub {
    std::vector<std::string> calls;
    pal4::BehaviorPathEndpointState* helper{};
    std::uint32_t first_result{};

    void WriteMemoryInt(std::uint32_t memory,
        std::string_view key, std::int32_t value) override {
        Check(memory == 0x9000u &&
              key == "AAB_NPC_memory_last_behavior_id" && value == 4,
              "class-four behavior save");
        calls.emplace_back("write-type");
    }
    float HelperSpeed(std::uint32_t helper) override {
        Check(helper == 0x6000u, "saved helper speed pointer");
        calls.emplace_back("speed");
        return 3.5f;
    }
    float AnimationTime(std::uint32_t animation) override {
        Check(animation == 0x5170u, "saved animation time pointer");
        calls.emplace_back("time");
        return 1.75f;
    }
    void WriteMemoryFloat(std::uint32_t memory,
        std::string_view key, float value) override {
        Check(memory == 0x9000u, "saved NPC memory pointer");
        if (key == "AAB_NPC_memory_speed")
            Check(value == 3.5f, "saved curve speed");
        else if (key == "AAB_NPC_memory_actiontime")
            Check(value == 1.75f, "saved curve action time");
        else
            Check(false, "unexpected curve save key");
        calls.emplace_back(key);
    }
    void RemoveGridElement(std::uint32_t grid,
        std::uint32_t helper_address) override {
        Check(grid == 0x7000u && helper_address == 0x6000u &&
              helper->words[0xb8u / 4u] == 0xaabbcc00u,
              "helper active byte clears before grid removal");
        calls.emplace_back("remove-grid");
    }
    std::uint32_t SelectAction(std::uint32_t animation,
        std::string_view name, float blend) override {
        Check(animation == 0x5170u && blend == 0.5f,
              "idle action address and blend");
        calls.emplace_back(name);
        return name == "C01" ? first_result : 0x55u;
    }
    std::uint32_t SetAnimationFlag(std::uint32_t animation,
        std::uint32_t flag) override {
        Check(animation == 0x5170u && flag == 1u,
              "idle animation flag");
        calls.emplace_back("flag");
        return 0x8765u;
    }
};

pal4::GameObjectState Manager() {
    pal4::GameObjectState manager{};
    manager.words[0x204u / 4u] = 0x7000u;
    manager.words[0x6d8u / 4u] = 0x8000u;
    manager.words[0x668u / 4u] = 0x9000u;
    manager.words[0x14u / 4u] = 0xa000u;
    return manager;
}

pal4::NpcPathEndpointTargetState Target() {
    pal4::NpcPathEndpointTargetState target{};
    target.words[0x04u / 4u] = 0x5000u;
    target.words[0x24u / 4u] = 0xdeadu;
    return target;
}

void Configure(RestoreRuntime& runtime) {
    runtime.children[0x8000u] = {0x8100u, 0x8200u};
    runtime.integers[{0x8100u, "NPCINFO_Behavior_ObjectType"}] = 1;
    runtime.integers[{0x8200u, "NPCINFO_Behavior_ObjectType"}] = 4;
    runtime.children[0x8200u] = {0x8300u, 0x8400u};
    runtime.names[0x8300u] = "other";
    runtime.names[0x8400u] = "AAB_NPC_parameters";
}

void MissingParameters() {
    auto manager = Manager();
    pal4::BehaviorPathEndpointState helper{};
    RestoreRuntime runtime;
    runtime.children[0x8000u] = {0x8100u};
    runtime.integers[{0x8100u, "NPCINFO_Behavior_ObjectType"}] = 3;
    Check(pal4::NpcCurveFollowTarget::Accept(Target(), manager,
              helper, 0x6000u, runtime) == 1u &&
          runtime.configured_path == 0xb000u &&
          runtime.scalars.empty() &&
          runtime.calls[0] == "path-name" &&
          runtime.calls[1] == "resolve-path" &&
          runtime.calls[2] == "grid-parameter" &&
          runtime.calls[3] == "configure-path",
          "missing class-four parameters return outer count after path setup");

    manager.words[0x6d8u / 4u] = 0u;
    runtime.calls.clear();
    Check(pal4::NpcCurveFollowTarget::Accept(Target(), manager,
              helper, 0x6000u, runtime) == 0u &&
          runtime.configured_path == 0u &&
          runtime.calls[0] == "grid-parameter",
          "null config skips pathway lookup");
}

void Restore(bool matching_behavior, bool c02_succeeds) {
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 0xaabbcc00u;
    RestoreRuntime runtime;
    Configure(runtime);
    runtime.floats[{0x8400u, "AABSeekOnPath_NPC_walk_max_speed"}] = 4.5f;
    runtime.floats[{0x9000u, "AAB_NPC_memory_speed"}] = 12.0f;
    runtime.integers[{0x9000u, "AAB_NPC_memory_last_behavior_id"}] =
        matching_behavior ? 4 : 2;
    runtime.floats[{0x9000u, "AAB_NPC_memory_actiontime"}] = 2.25f;
    runtime.c02_succeeds = c02_succeeds;
    const auto result = pal4::NpcCurveFollowTarget::Accept(
        Target(), Manager(), helper, 0x6000u, runtime);
    Check(result == (matching_behavior ? 0x1234u : 4u) &&
          runtime.scalars ==
              std::vector<std::pair<std::uint32_t, float>>{
                  {0x7cu, 4.5f}, {0x68u, 4.5f}} &&
          runtime.vectors ==
              std::vector<std::pair<std::uint32_t, Vector>>{
                  {0x14u, {7, 8, 9}}, {0x04u, {1, 2, 3}},
                  {0x0cu, {4, 5, 6}}, {0x1cu, {10, 11, 12}}} &&
          helper.words[0xb8u / 4u] == 0xaabbcc01u,
          "curve speed uses only two slots; transform and flag restore");
    auto expected = std::vector<std::string>{"C02"};
    if (!c02_succeeds) expected.emplace_back("Z02");
    Check(runtime.animations == expected &&
          runtime.animation_time ==
              (matching_behavior ? 2.25f : -1.0f),
          "animation fallback and behavior ID action-time gate");
    auto find = [&](std::string_view name) {
        for (std::size_t i = 0; i < runtime.calls.size(); ++i)
            if (runtime.calls[i] == name) return i;
        return runtime.calls.size();
    };
    Check(find("AAB_NPC_memory_speed") < find("transform") &&
          find("transform") < find("add-grid") &&
          find("add-grid") < find("last-behavior") &&
          (matching_behavior ? find("last-behavior") < find("set-time")
                             : find("set-time") == runtime.calls.size()),
          "memory speed is read without replacing curve speed");
}

void Save(bool first_succeeds) {
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 0xaabbcc01u;
    SaveRuntime runtime;
    runtime.helper = &helper;
    runtime.first_result = first_succeeds ? 0x77u : 0u;
    const auto result = pal4::NpcCurveFollowTarget::Release(
        Target(), Manager(), helper, 0x6000u, runtime);
    auto expected = std::vector<std::string>{
        "write-type", "speed", "AAB_NPC_memory_speed",
        "time", "AAB_NPC_memory_actiontime", "remove-grid", "C01"};
    if (!first_succeeds) expected.emplace_back("Z01");
    expected.emplace_back("flag");
    Check(result == 0x8765u && runtime.calls == expected &&
          helper.words[0xb8u / 4u] == 0xaabbcc00u,
          "curve save order, idle action fallback and final flag result");
}

}  // namespace

int main() {
    MissingParameters();
    Restore(true, true);
    Restore(true, false);
    Restore(false, true);
    Save(true);
    Save(false);
    std::cout << "NPC curve-follow target cases passed\n";
}
