#include "pal4/monster_path_endpoint_update.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMonsterPathEndpointUpdateRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> reads;
    pal4::BehaviorVector3 applied{};
    std::array<pal4::BehaviorVector3, 4> orientation{};
    float first_radius{};
    float second_radius{};
    std::uint32_t property_calls{};

    double ManagerDoubleProperty(std::uint32_t manager) override {
        Check(manager == 0x5000u, "manager property address");
        calls.emplace_back("property");
        ++property_calls;
        return 10.0;
    }
    pal4::BehaviorVector3 ReadHelperVector(
        std::uint32_t helper, std::uint32_t slot) override {
        Check(helper == 0x6000u, "helper vector address");
        calls.emplace_back("read:" + std::to_string(slot));
        const auto index = reads[slot]++;
        const auto base = static_cast<float>(slot + index * 10u);
        return {base + 1.0f, base + 2.0f, base + 3.0f};
    }
    void InitializeMovementAdapter() override {
        calls.emplace_back("adapter");
    }
    void CheckCollision(pal4::BehaviorVector3& position,
                        float first, float second) override {
        calls.emplace_back("collision");
        first_radius = first;
        second_radius = second;
        position[0] += 1.0f;
        position[1] += 2.0f;
        position[2] += 3.0f;
    }
    void ApplyHelperPosition(std::uint32_t helper,
        const pal4::BehaviorVector3& position) override {
        Check(helper == 0x6000u, "helper position address");
        calls.emplace_back("apply");
        applied = position;
    }
    void UpdateOrientation(std::uint32_t manager,
        const pal4::BehaviorVector3& one,
        const pal4::BehaviorVector3& two,
        const pal4::BehaviorVector3& three,
        const pal4::BehaviorVector3& position) override {
        Check(manager == 0x5000u, "orientation manager address");
        calls.emplace_back("orientation");
        orientation = {one, two, three, position};
    }
};

pal4::MonsterPathEndpointTargetState Target() {
    pal4::MonsterPathEndpointTargetState target{};
    target.words[0x1cu / 4u] = 0x5000u;
    return target;
}

void Process(const pal4::BehaviorPathEndpointState& helper,
             pal4::MonsterPathEndpointUpdateGlobals& globals,
             Runtime& runtime) {
    const auto target = Target();
    pal4::MonsterPathEndpointUpdate::Process(
        target, helper, 0x6000u, globals, runtime);
}

void LazyRadiiAndInactivePath() {
    pal4::BehaviorPathEndpointState helper{};
    pal4::MonsterPathEndpointUpdateGlobals globals{};
    globals.guard = 0x80u;
    Runtime runtime;
    Process(helper, globals, runtime);
    Check(globals.guard == 0x83u &&
              globals.first_radius > 3.49f &&
              globals.first_radius < 3.51f &&
              globals.second_radius > 25.29f &&
              globals.second_radius < 25.31f &&
              runtime.property_calls == 2u &&
              runtime.calls == std::vector<std::string>{
                  "property", "property"},
          "inactive helper initializes both radii and preserves guard bits");

    runtime.calls.clear();
    Process(helper, globals, runtime);
    Check(runtime.calls.empty() && runtime.property_calls == 2u,
          "already cached radii and inactive helper perform no calls");

    globals.guard = 0x81u;
    globals.first_radius = 7.0f;
    runtime.calls.clear();
    Process(helper, globals, runtime);
    Check(globals.guard == 0x83u && globals.first_radius == 7.0f &&
              runtime.calls == std::vector<std::string>{"property"},
          "each cached radius has an independent guard bit");
}

void ActivePositionAndOrientation() {
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 0x100u;
    pal4::MonsterPathEndpointUpdateGlobals globals{};
    Runtime runtime;
    Process(helper, globals, runtime);
    const pal4::BehaviorVector3 expected_position{26.0f, 38.0f, 50.0f};
    Check(runtime.reads == std::map<std::uint32_t, std::uint32_t>{
              {0x18u, 3u}, {0x10u, 3u}, {0x00u, 3u}, {0x08u, 3u}} &&
              globals.position == expected_position &&
              runtime.applied == expected_position &&
              globals.orientation_one ==
                  pal4::BehaviorVector3{17.0f, 28.0f, 39.0f} &&
              globals.orientation_two ==
                  pal4::BehaviorVector3{1.0f, 12.0f, 23.0f} &&
              globals.orientation_three ==
                  pal4::BehaviorVector3{9.0f, 20.0f, 31.0f} &&
              runtime.orientation ==
                  std::array<pal4::BehaviorVector3, 4>{
                      globals.orientation_one,
                      globals.orientation_two,
                      globals.orientation_three,
                      expected_position} &&
              runtime.calls == std::vector<std::string>{
                  "property", "property",
                  "read:24", "read:24", "read:24",
                  "adapter", "collision", "apply",
                  "read:16", "read:16", "read:16",
                  "read:0", "read:0", "read:0",
                  "read:8", "read:8", "read:8", "orientation"},
          "active path preserves repeated virtual reads and callback order");
}

void LinkableStub() {
    auto target = Target();
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xb8u / 4u] = 0x100u;
    pal4::MonsterPathEndpointUpdateGlobals globals{};
    pal4::MonsterPathEndpointUpdateRuntimeStub runtime;
    pal4::MonsterPathEndpointUpdate::Process(
        target, helper, 0x6000u, globals, runtime);
    Check(globals.guard == 3u &&
              globals.position == pal4::BehaviorVector3{},
          "movement, collision and orientation stubs link");
}

void C03IndependentCacheAndActiveUpdate() {
    auto target = Target();
    target.words[0x14u / 4u] = 0x5000u;
    target.words[0x1cu / 4u] = 0xdead0000u;
    pal4::MonsterPathEndpointUpdateGlobals c02_globals{};
    pal4::MonsterPathEndpointUpdateGlobals c03_globals{};
    c02_globals.guard = 3u;
    c02_globals.first_radius = 90.0f;
    c02_globals.second_radius = 91.0f;
    pal4::BehaviorPathEndpointState helper{};
    Runtime runtime;

    pal4::MonsterPathEndpointC03Update::Process(
        target, helper, 0x6000u, c03_globals, runtime);
    Check(c03_globals.guard == 3u &&
          c03_globals.first_radius > 3.49f &&
          c03_globals.first_radius < 3.51f &&
          c03_globals.second_radius > 25.29f &&
          c03_globals.second_radius < 25.31f &&
          c02_globals.first_radius == 90.0f &&
          c02_globals.second_radius == 91.0f &&
          runtime.calls == std::vector<std::string>{"property", "property"},
          "C03 initializes its own radii before inactive early return");

    helper.words[0xb8u / 4u] = 0x100u;
    runtime.calls.clear();
    pal4::MonsterPathEndpointC03Update::Process(
        target, helper, 0x6000u, c03_globals, runtime);
    Check(runtime.property_calls == 2u &&
          runtime.reads == std::map<std::uint32_t, std::uint32_t>{
              {0x18u, 3u}, {0x10u, 3u}, {0x00u, 3u}, {0x08u, 3u}} &&
          runtime.calls == std::vector<std::string>{
              "read:24", "read:24", "read:24",
              "adapter", "collision", "apply",
              "read:16", "read:16", "read:16",
              "read:0", "read:0", "read:0",
              "read:8", "read:8", "read:8", "orientation"} &&
          runtime.orientation[3] == c03_globals.position,
          "C03 repeats virtual reads and updates orientation in order");

    c03_globals.guard = 1u;
    c03_globals.first_radius = 17.0f;
    runtime.calls.clear();
    helper.words[0xb8u / 4u] = 0u;
    pal4::MonsterPathEndpointC03Update::Process(
        target, helper, 0x6000u, c03_globals, runtime);
    Check(c03_globals.guard == 3u &&
          c03_globals.first_radius == 17.0f &&
          runtime.calls == std::vector<std::string>{"property"},
          "C03 caches each radius independently");
}

void RandomPointIndependentCacheAndActiveUpdate() {
    auto target = Target();
    target.words[0x04u / 4u] = 0x5000u;
    target.words[0x1cu / 4u] = 0xdead0000u;
    pal4::MonsterPathEndpointUpdateGlobals c03_globals{};
    pal4::MonsterPathEndpointUpdateGlobals random_globals{};
    c03_globals.guard = 3u;
    c03_globals.first_radius = 90.0f;
    c03_globals.second_radius = 91.0f;
    pal4::BehaviorPathEndpointState helper{};
    Runtime runtime;

    pal4::MonsterRandomPointUpdate::Process(
        target, helper, 0x6000u, random_globals, runtime);
    Check(random_globals.guard == 3u &&
          random_globals.first_radius > 3.49f &&
          random_globals.first_radius < 3.51f &&
          random_globals.second_radius > 25.29f &&
          random_globals.second_radius < 25.31f &&
          c03_globals.first_radius == 90.0f &&
          c03_globals.second_radius == 91.0f &&
          runtime.calls == std::vector<std::string>{"property", "property"},
          "random target initializes independent radii before inactive return");

    helper.words[0xb8u / 4u] = 0x100u;
    runtime.calls.clear();
    pal4::MonsterRandomPointUpdate::Process(
        target, helper, 0x6000u, random_globals, runtime);
    Check(runtime.property_calls == 2u &&
          runtime.reads == std::map<std::uint32_t, std::uint32_t>{
              {0x18u, 3u}, {0x10u, 3u}, {0x00u, 3u}, {0x08u, 3u}} &&
          runtime.calls == std::vector<std::string>{
              "read:24", "read:24", "read:24",
              "adapter", "collision", "apply",
              "read:16", "read:16", "read:16",
              "read:0", "read:0", "read:0",
              "read:8", "read:8", "read:8", "orientation"} &&
          runtime.orientation[3] == random_globals.position,
          "random target repeats virtual reads and updates orientation");

    random_globals.guard = 1u;
    random_globals.first_radius = 17.0f;
    runtime.calls.clear();
    helper.words[0xb8u / 4u] = 0u;
    pal4::MonsterRandomPointUpdate::Process(
        target, helper, 0x6000u, random_globals, runtime);
    Check(random_globals.guard == 3u &&
          random_globals.first_radius == 17.0f &&
          runtime.calls == std::vector<std::string>{"property"},
          "random target guards each radius independently");
}

}  // namespace

int main() {
    LazyRadiiAndInactivePath();
    ActivePositionAndOrientation();
    LinkableStub();
    C03IndependentCacheAndActiveUpdate();
    RandomPointIndependentCacheAndActiveUpdate();
    std::cout << "monster path endpoint update cases passed\n";
}
