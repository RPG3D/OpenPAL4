#include "pal4/combat_action_console_window_input.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatActionConsoleWindowInputRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, std::int32_t> order{
        {0x8000u, 2}, {0x8100u, 0}, {0x8200u, 1},
        {0x8400u, 1}, {0x8500u, 0}, {0x8600u, 2}};
    std::vector<std::uint32_t> targets{0x8000u, 0x8100u, 0x8200u};
    std::vector<std::uint32_t> actors{0x8400u, 0x8500u, 0x8600u};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> redraw_windows;
    bool visible{};
    bool block_neighbor{};
    bool throw_filter{};
    std::uint32_t iterator_index{};
    std::uint32_t iterator_kind{};
    std::uint32_t released{};
    std::uint32_t redraws{};
    std::uint32_t selected{};
    std::uint32_t centered{};
    std::uint32_t actor_event{};
    std::uint32_t forwarded_key{};
    std::uint32_t started{};
    std::uint32_t confirmed{};

    Runtime() {
        words[0x11b8u] = 0x6000u;
        words[0x3010u] = 0u;
        words[0x11bcu] = 0x5000u;
        words[0x5000u] = 0x9000u;
        for (std::uint32_t index = 0u; index < 6u; ++index)
            words[0x1128u + 4u * index] = 0x7000u + 4u * index;
        for (std::uint32_t index = 0u; index < 4u; ++index) {
            words[0x5004u + 4u * index] = 0x5100u + 0x100u * index;
            words[0x5100u + 0x100u * index] = 0x9004u + 4u * index;
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte");
    }
    bool IsVisible(std::uint32_t window, bool recursive) override {
        Check(window == 0x6000u && !recursive, "panel visibility query");
        return visible;
    }
    bool IsDisabled(std::uint32_t window, bool recursive) override {
        Check(!recursive, "navigation disabled query");
        return block_neighbor && window >= 0x9004u;
    }
    void RequestRedraw(std::uint32_t window) override {
        ++redraws;
        redraw_windows.push_back(window);
    }
    void MouseAtCenter(std::uint32_t window) override {
        centered = window;
        calls.emplace_back("center");
    }
    std::uint32_t CombatSystem() override { return 0x4000u; }
    void StartCombat(std::uint32_t system) override {
        Check(system == 0x4000u, "start combat system");
        ++started;
    }
    void ConfirmCombat(std::uint32_t system) override {
        Check(system == 0x4000u, "confirm combat system");
        ++confirmed;
    }
    std::uint32_t CurrentTarget(std::uint32_t system) override {
        Check(system == 0x4000u, "current target system");
        return targets.empty() ? 0u : 0x8000u;
    }
    std::uint32_t AcquireTargetIterator(std::uint32_t target) override {
        Check(target == 0x8000u, "target iterator owner");
        iterator_kind = 1u;
        iterator_index = 0u;
        return 0xa000u;
    }
    std::uint32_t ActorManager() override { return 0x4300u; }
    std::uint32_t AcquireActorIterator(std::uint32_t collection) override {
        Check(collection == 0x4308u, "actor iterator collection");
        iterator_kind = 2u;
        iterator_index = 0u;
        return 0xa100u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == (iterator_kind == 1u ? 0xa000u : 0xa100u),
              "iterator handle");
        return iterator_index >=
               (iterator_kind == 1u ? targets.size() : actors.size());
    }
    std::uint32_t IteratorCurrent(std::uint32_t) override {
        return iterator_kind == 1u ? targets.at(iterator_index)
                                   : actors.at(iterator_index);
    }
    void IteratorNext(std::uint32_t) override { ++iterator_index; }
    void ReleaseIterator(std::uint32_t iterator,
                         std::uint32_t flags) override {
        Check(iterator == (iterator_kind == 1u ? 0xa000u : 0xa100u) &&
                  flags == 1u, "iterator deleting release");
        ++released;
    }
    bool IsValidTarget(std::uint32_t system,
                       std::uint32_t actor) override {
        Check(system == 0x4000u, "target filter system");
        if (throw_filter) throw std::runtime_error("filter");
        return actor != 0x8100u;
    }
    std::int32_t ActorOrder(std::uint32_t actor) override {
        return order.at(actor);
    }
    void SelectTarget(std::uint32_t system,
                      std::uint32_t actor) override {
        Check(system == 0x4000u, "target selection system");
        selected = actor;
    }
    void ForwardInputEvent(std::uint32_t key,
                           std::uint32_t event,
                           std::uint32_t flags) override {
        Check(key == 0x8b8b7cu && event == 0x3000u && flags == 1u,
              "key 19 script event");
        forwarded_key = key;
    }
    void ForwardActorEvent(std::uint32_t key,
                           std::uint32_t actor,
                           std::uint32_t flags) override {
        Check(key == 0x8b62dcu && flags == 1u,
              "actor selection script event");
        actor_event = actor;
    }
    std::uint8_t ForwardNavigation(std::uint32_t window,
                                   std::uint32_t event) override {
        Check(window == 0x1000u && event == 0x3000u,
              "navigation fallback arguments");
        calls.emplace_back("navigation");
        return 0x5au;
    }
};

std::uint8_t Handle(Runtime& runtime, std::uint32_t key) {
    runtime.words[0x3010u] = key;
    return pal4::CombatActionConsoleWindowInput::Handle(
        0x1000u, 0x3000u, runtime);
}

void CheckVisibleNavigation() {
    constexpr std::array<std::uint32_t, 4> keys{
        203u, 205u, 200u, 208u};
    for (std::uint32_t index = 0u; index < keys.size(); ++index) {
        Runtime runtime;
        runtime.visible = true;
        const auto next = 0x5100u + 0x100u * index;
        Check(Handle(runtime, keys[index]) == 1u &&
                  runtime.words[0x11bcu] == next &&
                  runtime.bytes[0x9004u + 4u * index + 0x4b1u] == 1u &&
                  runtime.redraws == 8u,
              "visible arrow follows its graph link and redraws selection");
        Runtime blocked;
        blocked.visible = true;
        blocked.block_neighbor = true;
        Check(Handle(blocked, keys[index]) == 1u &&
                  blocked.words[0x11bcu] == 0x5000u &&
                  blocked.redraws == 0u,
              "disabled neighbor leaves navigation unchanged");
    }
    Runtime no_current;
    no_current.visible = true;
    no_current.words[0x11bcu] = 0u;
    Check(Handle(no_current, 203u) == 1u &&
              no_current.redraws == 0u,
          "missing current node skips navigation");
    Runtime enter;
    enter.visible = true;
    Check(Handle(enter, 28u) == 1u && enter.centered == 0x9000u,
          "visible enter centers mouse on current node window");
}

void CheckHiddenCommands() {
    Runtime runtime;
    Check(Handle(runtime, 1u) == 1u && runtime.started == 1u,
          "hidden key 1 starts combat");
    Check(Handle(runtime, 28u) == 1u && runtime.confirmed == 1u,
          "hidden enter confirms combat");
    constexpr std::array<std::uint32_t, 4> keys{
        205u, 203u, 208u, 200u};
    constexpr std::array<std::uint32_t, 4> expected{
        0x8200u, 0x8200u, 0x8200u, 0x8200u};
    for (std::uint32_t index = 0u; index < keys.size(); ++index) {
        Runtime cycle;
        Check(Handle(cycle, keys[index]) == 1u &&
                  cycle.selected == expected[index] &&
                  cycle.released == 1u,
              "hidden arrow cycles sorted targets and releases iterator");
    }
    Runtime empty;
    empty.targets.clear();
    Check(Handle(empty, 205u) == 1u && empty.released == 0u,
          "missing current target skips iterator allocation");
}

void CheckSharedShortcuts() {
    Runtime runtime;
    Check(Handle(runtime, 19u) == 0x5au &&
              runtime.forwarded_key == 0x8b8b7cu,
          "key 19 forwards event then navigation result");
    constexpr std::array<std::uint32_t, 6> offsets{
        0x128u, 0x138u, 0x134u, 0x130u, 0x12cu, 0x13cu};
    for (std::uint32_t index = 0u; index < offsets.size(); ++index) {
        Runtime shortcut;
        Check(Handle(shortcut, 2u + index) == 0x5au &&
                  shortcut.centered == shortcut.words[0x1000u + offsets[index]],
              "number shortcut centers its mapped button");
    }
    constexpr std::array<std::uint32_t, 3> selected{
        0x8500u, 0x8400u, 0x8600u};
    for (std::uint32_t index = 0u; index < selected.size(); ++index) {
        Runtime type;
        Check(Handle(type, 59u + index) == 0x5au &&
                  type.actor_event == selected[index] &&
                  type.released == 1u,
              "F-key selects first actor of matching type");
    }
    Runtime visible;
    visible.visible = true;
    Check(Handle(visible, 19u) == 0x5au &&
              visible.forwarded_key == 0x8b8b7cu,
          "common shortcut also runs while panel is visible");
}

void CheckExceptionRelease() {
    Runtime runtime;
    runtime.throw_filter = true;
    bool caught = false;
    try {
        (void)Handle(runtime, 203u);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.released == 1u,
          "target filter exception releases iterator");
}

void CheckRefreshSelection() {
    Runtime runtime;
    for (std::uint32_t index = 0u; index < 6u; ++index)
        runtime.bytes[0x7000u + 4u * index + 0x4b1u] = 1u;
    Check(pal4::CombatActionConsoleWindowInput::RefreshSelection(
              0x1000u, runtime) == 1u &&
              runtime.redraws == 7u &&
              runtime.redraw_windows.back() == 0x9000u &&
              runtime.bytes[0x9000u + 0x4b1u] == 1u,
          "refresh clears six buttons then redraws current selection");
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        Check(runtime.bytes[0x7000u + 4u * index + 0x4b1u] == 0u &&
                  runtime.redraw_windows[index] == 0x7000u + 4u * index,
              "refresh clears and redraws every button in address order");
    }

    Runtime no_selection;
    no_selection.words[0x11bcu] = 0u;
    Check(pal4::CombatActionConsoleWindowInput::RefreshSelection(
              0x1000u, no_selection) == 1u &&
              no_selection.redraws == 6u,
          "refresh skips selected button when navigation node is null");
}

void CheckTargetOrderComparator() {
    Runtime runtime;
    runtime.order[0x8000u] = std::numeric_limits<std::int32_t>::min();
    runtime.order[0x8100u] = std::numeric_limits<std::int32_t>::max();
    Check(pal4::CombatActionConsoleWindowInput::TargetOrderLess(
              0x8000u, 0x8100u, runtime) &&
              !pal4::CombatActionConsoleWindowInput::TargetOrderLess(
                  0x8100u, 0x8000u, runtime),
          "target order compares signed virtual results");
    runtime.order[0x8100u] = runtime.order[0x8000u];
    Check(!pal4::CombatActionConsoleWindowInput::TargetOrderLess(
              0x8000u, 0x8100u, runtime),
          "equal target order values are not less");
}

void CheckActorArraySort() {
    for (const std::uint32_t count : {0u, 1u, 16u, 17u, 65u}) {
        Runtime runtime;
        std::vector<std::uint32_t> actors;
        for (std::uint32_t index = 0u; index < count; ++index) {
            const auto actor = 0xb000u + 4u * index;
            actors.push_back(actor);
            runtime.order[actor] = static_cast<std::int32_t>(count - index);
        }
        pal4::CombatActorArraySort::Sort(actors, runtime);
        Check(actors.size() == count, "actor sort preserves array size");
        for (std::uint32_t index = 1u; index < count; ++index)
            Check(runtime.order.at(actors[index - 1u]) <=
                      runtime.order.at(actors[index]),
                  "host actor sort preserves original virtual-order comparison across range sizes");
    }
    Runtime duplicate_runtime;
    std::vector<std::uint32_t> duplicates;
    for (std::uint32_t index = 0u; index < 65u; ++index) {
        const auto actor = 0xc000u + 4u * index;
        duplicates.push_back(actor);
        duplicate_runtime.order[actor] = static_cast<std::int32_t>(index % 3u);
    }
    const auto original = duplicates;
    pal4::CombatActorArraySort::Sort(duplicates, duplicate_runtime);
    Check(std::is_permutation(duplicates.begin(),duplicates.end(),original.begin()),
          "host sort preserves every actor including equivalent order keys");
    for (std::size_t index = 1u; index < duplicates.size(); ++index)
        Check(duplicate_runtime.order.at(duplicates[index - 1u]) <=
                  duplicate_runtime.order.at(duplicates[index]),
              "actor sort handles equal keys across large partitions");


}

}  // namespace

int main() {
    try {
        CheckVisibleNavigation();
        CheckHiddenCommands();
        CheckSharedShortcuts();
        CheckExceptionRelease();
        CheckRefreshSelection();
        CheckTargetOrderComparator();
        CheckActorArraySort();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
