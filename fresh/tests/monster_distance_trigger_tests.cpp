#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_distance_trigger.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
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

struct Runtime final : pal4::IMonsterDistanceTriggerRuntime {
    std::optional<double> distance;
    std::string current_animation{"idle"};
    bool select_c01{true};
    std::vector<std::string> calls;
    std::array<float, 3> delta{};

    double Distance(const std::array<float, 3>& input) override {
        calls.emplace_back("distance");
        delta = input;
        return distance.value_or(0.0);
    }
    void NotifyAt564(std::uint32_t address) override {
        Check(address == 0x5564u, "manager callback subobject address");
        calls.emplace_back("notify");
    }
    std::string AnimationName(std::uint32_t address) override {
        Check(address == 0x5170u, "animation subobject address");
        calls.emplace_back("name");
        return current_animation;
    }
    bool NameEqualsIgnoreCase(std::string_view left,
                              std::string_view right) override {
        calls.emplace_back(std::string("compare:") + std::string(right));
        pal4::MonsterDistanceTriggerRuntimeStub stub;
        return stub.NameEqualsIgnoreCase(left, right);
    }
    bool SelectAnimation(std::uint32_t address,
                         std::string_view name) override {
        Check(address == 0x5170u, "animation selection address");
        calls.emplace_back(std::string("select:") + std::string(name));
        return name == "C01" ? select_c01 : true;
    }
};

pal4::MonsterBehaviorManagerState Manager() {
    pal4::MonsterBehaviorManagerState manager{};
    manager.words[0xacu / 4u] = std::bit_cast<std::uint32_t>(10.0f);
    manager.words[0xb0u / 4u] = std::bit_cast<std::uint32_t>(20.0f);
    manager.words[0xb4u / 4u] = std::bit_cast<std::uint32_t>(30.0f);
    manager.words[0x730u / 4u] = 7u;
    manager.words[0x734u / 4u] = 8u;
    manager.words[0x738u / 4u] = 9u;
    return manager;
}

pal4::MonsterDistanceTriggerState Trigger(std::uint32_t kind) {
    return {1u, kind, {13.0f, 25.0f, 37.0f}};
}

std::uint32_t Process(pal4::MonsterDistanceTriggerState& trigger,
                      pal4::MonsterBehaviorManagerState& manager,
                      Runtime& runtime) {
    return pal4::MonsterDistanceTrigger::Process(
        trigger, 0x4000u, manager, 0x5000u, runtime);
}

void InactiveAndDelta() {
    auto trigger = Trigger(0u);
    auto manager = Manager();
    Runtime runtime;
    trigger.active = 0u;
    Check(Process(trigger, manager, runtime) == 0x4000u &&
              runtime.calls.empty() &&
              manager.words[0x730u / 4u] == 7u,
          "inactive trigger returns without distance calculation");
    trigger.active = 1u;
    runtime.distance = 50.0;
    Check(Process(trigger, manager, runtime) == 0x4000u &&
              runtime.delta == std::array<float, 3>{3.0f, 5.0f, 7.0f} &&
              trigger.active == 1u &&
              manager.words[0x730u / 4u] == 7u,
          "active trigger uses XYZ delta and leaves distant kind zero intact");
}

void KindTwoThreshold() {
    for (const auto [distance, should_set] : {
             std::pair{150.0, true}, std::pair{150.001, false},
             std::pair{std::numeric_limits<double>::quiet_NaN(), false}}) {
        auto trigger = Trigger(2u);
        auto manager = Manager();
        Runtime runtime;
        runtime.distance = distance;
        Check(Process(trigger, manager, runtime) == 0x4000u &&
                  trigger.active == 1u &&
                  manager.words[0x730u / 4u] ==
                      (should_set ? 2u : 7u) &&
                  manager.words[0x734u / 4u] ==
                      (should_set ? std::bit_cast<std::uint32_t>(10.0f)
                                  : 8u) &&
                  manager.words[0x738u / 4u] ==
                      (should_set ? 0u : 9u) &&
                  runtime.calls == std::vector<std::string>{"distance"},
              "kind two uses ordered inclusive 150 threshold");
    }
}

void UnknownKindClearsState() {
    auto trigger = Trigger(3u);
    auto manager = Manager();
    Runtime runtime;
    runtime.distance = std::numeric_limits<double>::quiet_NaN();
    Check(Process(trigger, manager, runtime) == 0x4000u &&
              trigger.active == 1u &&
              manager.words[0x730u / 4u] == 0u &&
              manager.words[0x734u / 4u] == 0u &&
              manager.words[0x738u / 4u] == 0u &&
              runtime.calls == std::vector<std::string>{"distance"},
          "other kind clears all three state words regardless of distance");
}

void KindZeroAnimationPaths() {
    for (const auto [current, select_c01, expected] : {
             std::tuple{"c01", true,
                        std::vector<std::string>{"distance", "notify",
                            "name", "compare:C01"}},
             std::tuple{"z01", true,
                        std::vector<std::string>{"distance", "notify",
                            "name", "compare:C01", "name", "compare:Z01"}},
             std::tuple{"idle", true,
                        std::vector<std::string>{"distance", "notify",
                            "name", "compare:C01", "name", "compare:Z01",
                            "select:C01"}},
             std::tuple{"idle", false,
                        std::vector<std::string>{"distance", "notify",
                            "name", "compare:C01", "name", "compare:Z01",
                            "select:C01", "select:Z01"}}}) {
        auto trigger = Trigger(0u);
        auto manager = Manager();
        Runtime runtime;
        runtime.distance = 40.0;
        runtime.current_animation = current;
        runtime.select_c01 = select_c01;
        Check(Process(trigger, manager, runtime) == 0x4000u &&
                  trigger.active == 0u &&
                  manager.words[0x730u / 4u] == 1u &&
                  manager.words[0x734u / 4u] ==
                      std::bit_cast<std::uint32_t>(5.0f) &&
                  manager.words[0x738u / 4u] == 0u &&
                  runtime.calls == expected,
              "kind zero sets state and chooses animation in observed order");
    }
    for (const auto distance : {
             40.001, std::numeric_limits<double>::quiet_NaN()}) {
        auto trigger = Trigger(0u);
        auto manager = Manager();
        Runtime runtime;
        runtime.distance = distance;
        Check(Process(trigger, manager, runtime) == 0x4000u &&
                  trigger.active == 1u &&
                  manager.words[0x730u / 4u] == 7u &&
                  runtime.calls == std::vector<std::string>{"distance"},
              "kind zero rejects greater and unordered distance");
    }
}

void StubDistanceAndLink() {
    pal4::MonsterDistanceTriggerRuntimeStub runtime;
    Check(runtime.Distance({3.0f, 4.0f, 0.0f}) == 5.0 &&
              runtime.NameEqualsIgnoreCase("c01", "C01"),
          "default distance and ASCII animation comparison link");
    auto trigger = Trigger(0u);
    auto manager = Manager();
    Check(pal4::MonsterDistanceTrigger::Process(
              trigger, 0x4000u, manager, 0x5000u, runtime) == 0x4000u,
          "default external animation services remain linkable");
}

}  // namespace

int main() {
    InactiveAndDelta();
    KindTwoThreshold();
    UnknownKindClearsState();
    KindZeroAnimationPaths();
    StubDistanceAndLink();
    std::cout << "monster distance trigger cases passed\n";
}
