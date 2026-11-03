#include "pal4/ai_action_task.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kActor = 0x5000u;
constexpr std::uint32_t kRequested = 0x6000u;
constexpr std::uint32_t kIterator = 0x7000u;
constexpr std::uint32_t kMember1 = 0x8000u;
constexpr std::uint32_t kMember2 = 0x8100u;

struct Runtime final : pal4::IAiActionTargetSelectionRuntime {
    std::vector<std::uint32_t> members{kMember1, kMember2};
    std::size_t cursor = 0u;
    pal4::AiActorGroup group = pal4::AiActorGroup::Team;
    bool throw_build = false;
    bool throw_select = false;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kTask + 4u, "selector reads task actor");
        calls.emplace_back("actor");
        return kActor;
    }
    std::uint32_t CreateActorIterator(
        std::uint32_t actor, pal4::AiActorGroup which) override {
        Check(actor == kActor, "iterator uses task actor");
        group = which;
        cursor = 0u;
        calls.emplace_back(which == pal4::AiActorGroup::Team
                               ? "team" : "enemy");
        return kIterator;
    }
    bool IteratorEnd(std::uint32_t iterator) override {
        Check(iterator == kIterator, "iterator end handle");
        return cursor == members.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        Check(iterator == kIterator && cursor < members.size(),
              "iterator current handle");
        calls.emplace_back("current");
        return members[cursor];
    }
    void IteratorNext(std::uint32_t iterator) override {
        Check(iterator == kIterator && cursor < members.size(),
              "iterator next handle");
        ++cursor;
        calls.emplace_back("next");
    }
    void ReleaseIterator(std::uint32_t iterator) noexcept override {
        if (iterator == kIterator) calls.emplace_back("release");
    }
    bool IsDead(std::uint32_t actor) override {
        Check(actor == kMember1 || actor == kMember2,
              "special selection probes candidate death state");
        calls.emplace_back("dead");
        return actor == kMember2;
    }
    std::vector<std::uint32_t> BuildPool(
        std::uint32_t task, std::uint32_t iterator,
        std::uint32_t mode) override {
        Check(task == kTask && iterator == kIterator && mode == 9u,
              "pool receives task, iterator and mode");
        calls.emplace_back("build");
        if (throw_build) throw std::runtime_error("build failure");
        return {0xa000u};
    }
    std::uint32_t SelectTarget(
        std::uint32_t task,
        const std::vector<std::uint32_t>& candidates,
        std::uint32_t requested, std::uint32_t mode,
        std::uint32_t state) override {
        Check(task == kTask && requested == kRequested && mode == 9u,
              "final selection forwards task, target and mode");
        Check(candidates == (state == 5u
                                 ? std::vector<std::uint32_t>{kMember2}
                                 : std::vector<std::uint32_t>{0xa000u}),
              "final selection receives expected candidates");
        calls.emplace_back("select");
        if (throw_select) throw std::runtime_error("select failure");
        return 0xb000u + state;
    }
};

std::uint32_t Select(std::uint32_t category,
                     std::uint32_t state,
                     Runtime& runtime) {
    return pal4::AiActionTargetSelection::Select(
        kTask, category, state, kRequested, 9u, runtime);
}

void Categories() {
    Runtime runtime;
    Check(Select(0u, 7u, runtime) == kActor &&
          runtime.calls == std::vector<std::string>{"actor"},
          "category zero returns task actor directly");
    runtime = Runtime{};
    Check(Select(9u, 7u, runtime) == 0u &&
          runtime.calls.empty(),
          "unknown category returns null without actor services");

    for (const auto category : {1u, 2u, 3u, 4u}) {
        runtime = Runtime{};
        Check(Select(category, 7u, runtime) == 0xb007u &&
              runtime.group == (category % 2u == 1u
                                    ? pal4::AiActorGroup::Team
                                    : pal4::AiActorGroup::Enemy) &&
              runtime.calls == std::vector<std::string>{
                  "actor", category % 2u == 1u ? "team" : "enemy",
                  "build", "release", "select"},
              "categories 1/3 use team and 2/4 use enemy pool");
    }
}

void SpecialAndCleanup() {
    Runtime runtime;
    Check(Select(9u, 5u, runtime) == 0xb005u &&
          runtime.group == pal4::AiActorGroup::Team &&
          runtime.calls == std::vector<std::string>{
              "actor", "team", "current", "dead", "next",
              "current", "dead", "next", "select", "release"},
          "state five overrides category and retains dead teammates");

    runtime = Runtime{};
    runtime.throw_build = true;
    bool threw = false;
    try {
        (void)Select(2u, 7u, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls.back() == "release",
          "pool construction exception releases enemy iterator");

    runtime = Runtime{};
    runtime.throw_select = true;
    threw = false;
    try {
        (void)Select(1u, 5u, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls.back() == "release",
          "special selection exception releases team iterator");
}

}  // namespace

int main() {
    try {
        Categories();
        SpecialAndCleanup();
        std::cout << "ai_action_target_selection_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
