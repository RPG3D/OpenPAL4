#pragma once

#include "pal4/ui_navigation_list.h"
#include <cstdint>
#include <vector>

namespace pal4 {

class ICombatActionConsoleWindowInputRuntime : public IUiNavigationSelectionRuntime {
public:
    virtual ~ICombatActionConsoleWindowInputRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t window,
                                         bool recursive) = 0;
    [[nodiscard]] virtual bool IsDisabled(std::uint32_t window,
                                          bool recursive) = 0;
    virtual void RequestRedraw(std::uint32_t window) = 0;
    virtual void MouseAtCenter(std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    virtual void StartCombat(std::uint32_t system) = 0;
    virtual void ConfirmCombat(std::uint32_t system) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentTarget(
        std::uint32_t system) = 0;
    [[nodiscard]] virtual std::uint32_t AcquireTargetIterator(
        std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint32_t ActorManager() = 0;
    [[nodiscard]] virtual std::uint32_t AcquireActorIterator(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    virtual void ReleaseIterator(std::uint32_t iterator,
                                 std::uint32_t flags) = 0;
    [[nodiscard]] virtual bool IsValidTarget(std::uint32_t system,
                                             std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t ActorOrder(
        std::uint32_t actor) = 0;
    virtual void SelectTarget(std::uint32_t system,
                              std::uint32_t actor) = 0;
    virtual void ForwardInputEvent(std::uint32_t key,
                                   std::uint32_t event,
                                   std::uint32_t flags) = 0;
    virtual void ForwardActorEvent(std::uint32_t key,
                                   std::uint32_t actor,
                                   std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint8_t ForwardNavigation(
        std::uint32_t window, std::uint32_t event) = 0;
};

class CombatActionConsoleWindowInput final {
public:
    // IDA 0x541bd0.
    [[nodiscard]] static std::uint8_t Handle(
        std::uint32_t window, std::uint32_t event,
        ICombatActionConsoleWindowInputRuntime& runtime);
    // IDA 0x542600 and 0x542650.
    [[nodiscard]] static std::uint8_t RefreshSelection(
        std::uint32_t navigation, IUiNavigationSelectionRuntime& runtime);
    [[nodiscard]] static bool TargetOrderLess(
        std::uint32_t left, std::uint32_t right,
        ICombatActionConsoleWindowInputRuntime& runtime);
};

class CombatActorArraySort final {
public:
    // IDA542820 caller contract: host std::sort with original virtual-order comparator.
    // Library partition/insertion helpers542B50/542C70 have no private runtime API.
    static void Sort(std::vector<std::uint32_t>& actors,
                     ICombatActionConsoleWindowInputRuntime& runtime);
};

} // namespace pal4
