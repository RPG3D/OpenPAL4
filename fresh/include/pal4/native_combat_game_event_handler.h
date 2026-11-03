#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeGameDatabase;
class NativeScriptEventRouter;
class ICombatGameEventIterationRuntime;
// Owns620-byte GameEventHandler, CombatPrize records and17 subscriptions.
// CombatSystem520/router are borrowed. Actor queries are separately bound;
// callbacks reject missing actor services. Close before dependencies retire.
class NativeCombatGameEventHandler final {
public:
    NativeCombatGameEventHandler(const NativeGameDatabase&,NativeScriptEventRouter&,std::uint32_t combat);
    // Publish only after successful construction into the actual CombatSystem
    // member416. Borrowed slot/storage must outlive this owner. Close clears it
    // after unsubscribing; duplicate/foreign slots are rejected before mutation.
    NativeCombatGameEventHandler(const NativeGameDatabase&,NativeScriptEventRouter&,
        std::uint32_t combat,std::uint32_t& member_slot);
    ~NativeCombatGameEventHandler();
    void BindActors(ICombatGameEventIterationRuntime*);
    void Reset();
    void Subscribe();
    void Close();
    [[nodiscard]] std::uint32_t Address() const;
    [[nodiscard]] std::size_t SubscriptionCount() const;
    [[nodiscard]] std::uint32_t Total();
    [[nodiscard]] std::uint32_t Rank(std::int32_t score);
    std::uint32_t UpdateStat(std::uint32_t stat,std::uint32_t increment);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
