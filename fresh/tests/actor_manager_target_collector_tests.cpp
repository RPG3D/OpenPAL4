#include "pal4/actor_manager_target_collector.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t collection = 0x2000u;
constexpr std::uint32_t iterator = 0x3000u;

struct Runtime final : pal4::IActorManagerTargetCollectorRuntime {
    std::uint32_t mode = 0u;
    std::array<std::uint32_t, 7> types{0u, 1u, 2u, 3u, 4u, 5u, 6u};
    std::vector<std::string> trace;
    std::vector<std::uint32_t> selected;
    std::size_t index = 0;
    std::uint32_t type_calls = 0;

    std::uint32_t SelectionMode(std::uint32_t subject) override {
        Check(subject == actor, "selection mode actor");
        trace.emplace_back("mode");
        return mode;
    }
    std::uint32_t CreateIterator(std::uint32_t subject) override {
        Check(subject == actor, "iterator source actor");
        trace.emplace_back("create");
        return iterator;
    }
    bool AtEnd(std::uint32_t handle) override {
        Check(handle == iterator, "iterator end receiver");
        trace.emplace_back("end");
        return index == types.size();
    }
    std::uint32_t Current(std::uint32_t handle) override {
        Check(handle == iterator && index < types.size(),
              "iterator current receiver");
        trace.emplace_back("current");
        return 0x4000u + static_cast<std::uint32_t>(index);
    }
    std::uint32_t TargetType(std::uint32_t target) override {
        const auto offset = target - 0x4000u;
        Check(offset < types.size(), "target type receiver");
        ++type_calls;
        return types[offset];
    }
    void InsertDirect(std::uint32_t output,
                      std::uint32_t target) override {
        Check(output == collection, "direct collection");
        trace.emplace_back("direct");
        selected.push_back(target - 0x4000u);
    }
    void InsertEvent(std::uint32_t output,
                     std::uint32_t target) override {
        Check(output == collection, "event collection");
        trace.emplace_back("event");
        selected.push_back(target - 0x4000u);
    }
    void Advance(std::uint32_t handle) override {
        Check(handle == iterator, "iterator advance receiver");
        ++index;
        trace.emplace_back("advance");
    }
    std::uint32_t DestroyIterator(std::uint32_t handle) override {
        Check(handle == iterator, "iterator destroy receiver");
        trace.emplace_back("destroy");
        return 0x77u;
    }
};

void AllModes() {
    constexpr std::array expected{
        std::array<std::uint32_t, 2>{1u, 99u},
        std::array<std::uint32_t, 2>{0u, 2u},
        std::array<std::uint32_t, 2>{1u, 99u},
        std::array<std::uint32_t, 2>{4u, 99u},
        std::array<std::uint32_t, 2>{3u, 5u},
        std::array<std::uint32_t, 2>{4u, 99u}};
    for (std::uint32_t mode = 0; mode <= 5u; ++mode) {
        Runtime runtime;
        runtime.mode = mode;
        Check(pal4::ActorManagerTargetCollector::Collect(
                  actor, collection, runtime) == 0x77u &&
              runtime.index == runtime.types.size() &&
              runtime.trace.front() == "mode" &&
              runtime.trace[1] == "create" &&
              runtime.trace.back() == "destroy" &&
              runtime.selected.size() ==
                  (expected[mode][1] == 99u ? 1u : 2u),
              "mode scans every candidate and destroys iterator");
        for (std::size_t i = 0; i < runtime.selected.size(); ++i) {
            Check(runtime.selected[i] == expected[mode][i],
                  "mode selects exact target types in source order");
        }
        std::size_t insert_count = 0;
        for (const auto& step : runtime.trace) {
            Check(step != (mode == 0u ? "event" : "direct"),
                  "mode uses correct insertion path");
            if (step == (mode == 0u ? "direct" : "event")) {
                ++insert_count;
            }
        }
        Check(insert_count == runtime.selected.size() &&
              runtime.trace[2] == "end" &&
              runtime.trace[3] == "current" &&
              runtime.type_calls ==
                  (mode == 1u || mode == 4u ? 13u : 7u),
              "iterator and target type virtual call shape");
    }
}

void UnsupportedMode() {
    Runtime runtime;
    runtime.mode = 6u;
    Check(pal4::ActorManagerTargetCollector::Collect(
              actor, collection, runtime) == 0x77u &&
          runtime.trace == std::vector<std::string>{
              "mode", "create", "destroy"} &&
          runtime.selected.empty(),
          "unsupported selection mode skips scan but destroys iterator");
}

void AllCandidates() {
    Runtime runtime;
    Check(pal4::ActorManagerTargetCollector::CollectAll(
              actor, collection, runtime) == 0x77u &&
          runtime.selected == std::vector<std::uint32_t>{
              0u, 1u, 2u, 3u, 4u, 5u, 6u} &&
          runtime.type_calls == 0u &&
          runtime.trace.front() == "create" &&
          runtime.trace.back() == "destroy" &&
          runtime.trace.size() == 31u,
          "all-candidate route inserts every target without mode or type query");
    for (const auto& step : runtime.trace) {
        Check(step != "mode" && step != "direct",
              "all-candidate route uses event insertion only");
    }
}

}  // namespace

int main() {
    try {
        AllModes();
        UnsupportedMode();
        AllCandidates();
        std::cout << "actor_manager_target_collector_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
