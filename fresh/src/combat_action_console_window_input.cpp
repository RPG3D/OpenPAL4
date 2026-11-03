#include "pal4/combat_action_console_window_input.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

namespace pal4 {
namespace {

constexpr std::uint32_t kCycleEvent = 0x8b8b7cu;
constexpr std::uint32_t kActorEvent = 0x8b62dcu;

bool Less(const std::uint32_t left, const std::uint32_t right,
          ICombatActionConsoleWindowInputRuntime& runtime) {
    return CombatActionConsoleWindowInput::TargetOrderLess(
        left, right, runtime);
}

std::uint8_t MoveSelection(
    const std::uint32_t window, const std::uint32_t link_offset,
    ICombatActionConsoleWindowInputRuntime& runtime) {
    const auto current = runtime.ReadWord(window + 0x1bcu);
    if (current == 0u) return 1u;
    const auto next = runtime.ReadWord(current + link_offset);
    if (next == 0u ||
        runtime.IsDisabled(runtime.ReadWord(next), false)) return 1u;

    for (std::uint32_t index = 0u; index < 6u; ++index) {
        const auto button = runtime.ReadWord(window + 0x128u + 4u * index);
        runtime.WriteByte(button + 0x4b1u, 0u);
        runtime.RequestRedraw(button);
    }
    const auto old_button = runtime.ReadWord(current);
    runtime.WriteByte(old_button + 0x4b1u, 0u);
    runtime.RequestRedraw(old_button);
    runtime.WriteWord(window + 0x1bcu, next);
    const auto new_button = runtime.ReadWord(next);
    runtime.WriteByte(new_button + 0x4b1u, 1u);
    runtime.RequestRedraw(new_button);
    return 1u;
}

std::uint8_t CycleTarget(
    const std::uint32_t key,
    ICombatActionConsoleWindowInputRuntime& runtime) {
    const auto system = runtime.CombatSystem();
    const auto current = runtime.CurrentTarget(system);
    if (current == 0u) return 1u;

    const auto iterator = runtime.AcquireTargetIterator(current);
    std::vector<std::uint32_t> candidates;
    try {
        while (!runtime.IteratorAtEnd(iterator)) {
            const auto actor = runtime.IteratorCurrent(iterator);
            if (key == 205u || runtime.IsValidTarget(
                                   runtime.CombatSystem(), actor))
                candidates.push_back(actor);
            runtime.IteratorNext(iterator);
        }
        CombatActorArraySort::Sort(candidates, runtime);
        for (std::size_t index = 0u; index < candidates.size(); ++index) {
            if (candidates[index] != current) continue;
            const auto next = key == 205u || key == 208u
                                  ? (index + candidates.size() - 1u) %
                                        candidates.size()
                                  : (index + 1u) % candidates.size();
            runtime.SelectTarget(runtime.CombatSystem(), candidates[next]);
        }
    } catch (...) {
        if (iterator != 0u) runtime.ReleaseIterator(iterator, 1u);
        throw;
    }
    if (iterator != 0u) runtime.ReleaseIterator(iterator, 1u);
    return 1u;
}

void SelectActorType(
    const std::int32_t desired,
    ICombatActionConsoleWindowInputRuntime& runtime) {
    const auto iterator = runtime.AcquireActorIterator(
        runtime.ActorManager() + 8u);
    try {
        while (!runtime.IteratorAtEnd(iterator)) {
            const auto actor = runtime.IteratorCurrent(iterator);
            if (runtime.ActorOrder(actor) == desired) {
                runtime.ForwardActorEvent(kActorEvent, actor, 1u);
                break;
            }
            runtime.IteratorNext(iterator);
        }
    } catch (...) {
        if (iterator != 0u) runtime.ReleaseIterator(iterator, 1u);
        throw;
    }
    if (iterator != 0u) runtime.ReleaseIterator(iterator, 1u);
}

}  // namespace

std::uint8_t CombatActionConsoleWindowInput::Handle(
    const std::uint32_t window, const std::uint32_t event,
    ICombatActionConsoleWindowInputRuntime& runtime) {
    const auto panel = runtime.ReadWord(window + 0x1b8u);
    const auto visible = runtime.IsVisible(panel, false);
    const auto key = runtime.ReadWord(event + 0x10u);
    if (visible) {
        switch (key) {
        case 203u: return MoveSelection(window, 4u, runtime);
        case 205u: return MoveSelection(window, 8u, runtime);
        case 200u: return MoveSelection(window, 12u, runtime);
        case 208u: return MoveSelection(window, 16u, runtime);
        case 28u: {
            const auto current = runtime.ReadWord(window + 0x1bcu);
            if (current != 0u)
                runtime.MouseAtCenter(runtime.ReadWord(current));
            return 1u;
        }
        default: break;
        }
    } else {
        if (key == 1u) {
            runtime.StartCombat(runtime.CombatSystem());
            return 1u;
        }
        if (key == 28u) {
            runtime.ConfirmCombat(runtime.CombatSystem());
            return 1u;
        }
        if (key == 200u || key == 203u || key == 205u || key == 208u)
            return CycleTarget(key, runtime);
    }

    if (key == 19u) runtime.ForwardInputEvent(kCycleEvent, event, 1u);
    constexpr std::array<std::uint32_t, 6> kDirectButtons{
        0x128u, 0x138u, 0x134u, 0x130u, 0x12cu, 0x13cu};
    if (key >= 2u && key <= 7u)
        runtime.MouseAtCenter(runtime.ReadWord(
            window + kDirectButtons[key - 2u]));
    if (key >= 59u && key <= 61u)
        SelectActorType(static_cast<std::int32_t>(key - 59u), runtime);
    return runtime.ForwardNavigation(window, event);
}

std::uint8_t CombatActionConsoleWindowInput::RefreshSelection(
    const std::uint32_t window,
    IUiNavigationSelectionRuntime& runtime) {
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        const auto button = runtime.ReadWord(window + 0x128u + 4u * index);
        runtime.WriteByte(button + 0x4b1u, 0u);
        runtime.RequestRedraw(button);
    }
    const auto current = runtime.ReadWord(window + 0x1bcu);
    if (current != 0u) {
        const auto button = runtime.ReadWord(current);
        runtime.WriteByte(button + 0x4b1u, 1u);
        runtime.RequestRedraw(button);
    }
    return 1u;
}

bool CombatActionConsoleWindowInput::TargetOrderLess(
    const std::uint32_t left, const std::uint32_t right,
    ICombatActionConsoleWindowInputRuntime& runtime) {
    const auto left_order = runtime.ActorOrder(left);
    return left_order < runtime.ActorOrder(right);
}

void CombatActorArraySort::Sort(
    std::vector<std::uint32_t>& actors,
    ICombatActionConsoleWindowInputRuntime& runtime) {
    // The original generic pointer/comparator sort is library machinery.
    // Keep the game's virtual-order comparator and use the selected host STL.
    std::sort(actors.begin(),actors.end(),[&](auto a,auto b){return Less(a,b,runtime);});
}

} // namespace pal4
