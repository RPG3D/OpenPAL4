#include "pal4/ai_action_task.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kAnimation = 0x5000u;
constexpr std::uint32_t kBase = 0x10000u;

struct Runtime final : pal4::IAiActionTargetCatalogRuntime {
    std::uint32_t animation = kAnimation;
    std::uint32_t base = kBase;
    std::uint32_t count = 3u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t PlayAnimation(std::uint32_t task) override {
        Check(task == kTask, "play uses task");
        calls.emplace_back("play");
        return animation;
    }
    std::uint32_t ProcessTargetSelection(
        std::uint32_t task, std::uint32_t item) override {
        Check(task == kTask && item == kAnimation,
              "selection process receives animation");
        calls.emplace_back("process-selection");
        return 33u;
    }
    std::uint32_t SelectTarget(
        std::uint32_t task, std::uint32_t item) override {
        Check(task == kTask && item == kAnimation,
              "target selection receives animation");
        calls.emplace_back("select-target");
        return 22u;
    }
    std::uint32_t AnimationId(std::uint32_t task) override {
        Check(task == kTask, "animation ID uses task");
        calls.emplace_back("animation-id");
        return 11u;
    }
    std::uint32_t RecordBase() override {
        calls.emplace_back("base");
        return base;
    }
    std::uint32_t RecordCount() override {
        calls.emplace_back("count");
        return count;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words[address];
    }
    void ReportMissing(std::uint32_t item) override {
        Check(item == kAnimation, "diagnostic receives animation");
        calls.emplace_back("missing");
    }
};

void SetRecord(Runtime& runtime, std::uint32_t index,
               std::uint32_t animation_id,
               std::uint32_t selection_id,
               std::uint32_t target_id) {
    const auto address = kBase +
        index * pal4::AiActionTargetCatalog::kRecordSize;
    runtime.words[address + 4u] = animation_id;
    runtime.words[address + 8u] = selection_id;
    runtime.words[address + 12u] = target_id;
}

void MatchAndOrder() {
    Runtime runtime;
    SetRecord(runtime, 0u, 9u, 33u, 22u);
    SetRecord(runtime, 1u, 11u, 33u, 22u);
    SetRecord(runtime, 2u, 11u, 33u, 22u);
    Check(pal4::AiActionTargetCatalog::ChooseRecord(
              kTask, runtime) ==
              kBase + pal4::AiActionTargetCatalog::kRecordSize &&
          runtime.calls == std::vector<std::string>{
              "play", "process-selection", "select-target",
              "animation-id", "base", "count", "read", "read",
              "read", "read"},
          "first matching 68-byte record and original query order");

    runtime = Runtime{};
    const pal4::AiActionTargetKey key{11u, 22u, 33u};
    SetRecord(runtime, 0u, 11u, 99u, 22u);
    Check(!pal4::AiActionTargetCatalog::CompareTargets(
              key, kBase, runtime) && runtime.calls.size() == 2u,
          "selection mismatch short circuits before target field");
    runtime.calls.clear();
    SetRecord(runtime, 0u, 11u, 33u, 44u);
    Check(!pal4::AiActionTargetCatalog::CompareTargets(
              key, kBase, runtime) && runtime.calls.size() == 3u,
          "target mismatch reads third record field");
}

void EmptyAndMissing() {
    Runtime runtime;
    runtime.animation = 0u;
    Check(pal4::AiActionTargetCatalog::ChooseRecord(
              kTask, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"play"},
          "failed animation lookup returns without scanning or diagnosis");

    runtime = Runtime{};
    runtime.count = 0u;
    Check(pal4::AiActionTargetCatalog::ChooseRecord(
              kTask, runtime) == 0u &&
          runtime.calls.back() == "missing",
          "empty record list reports missing animation");

    runtime = Runtime{};
    runtime.base = 0u;
    Check(pal4::AiActionTargetCatalog::ChooseRecord(
              kTask, runtime) == 0u &&
          runtime.calls.back() == "missing",
          "null record base reports missing animation");

    runtime = Runtime{};
    SetRecord(runtime, 0u, 11u, 33u, 55u);
    SetRecord(runtime, 1u, 11u, 55u, 22u);
    SetRecord(runtime, 2u, 77u, 33u, 22u);
    Check(pal4::AiActionTargetCatalog::ChooseRecord(
              kTask, runtime) == 0u &&
          runtime.calls.back() == "missing",
          "record field order rejects every near match");
}

}  // namespace

int main() {
    try {
        MatchAndOrder();
        EmptyAndMissing();
        std::cout << "ai_action_target_catalog_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
