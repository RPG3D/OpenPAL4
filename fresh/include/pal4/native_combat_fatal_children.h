#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <functional>
#include "pal4/native_combat_fatal_visual_services.h"
namespace pal4 {
class ICombatScriptTaskRuntime;
class ICombatStatusTaskRuntime;
class ICombatStatusActorRuntime;
class ICombatActionTaskRuntime;
class IGomTaskBaseRuntime;
class IGomTaskContainerRuntime;
class NativeBaseObjectMemory;
class INativeCombatFinalHitMessage;
// Fatal children plus shared reaction animation-wait/preparation children.
// All borrowed actor/script/status/visual services outlive the owner.
// The caller owns the containing GOM sentinel/list and destroys it first.
class NativeCombatFatalChildren final {
public:
    using PrepareAction=std::function<void(std::uint32_t actor,std::uint32_t flag)>;
    // FendOff impact only, borrowing actual source508(target) routing.
    explicit NativeCombatFatalChildren(ICombatStatusActorRuntime&);
    // Mixed reaction GOM: caller owns the shared object/name/list allocation domain.
    NativeCombatFatalChildren(ICombatStatusActorRuntime&,ICombatActionTaskRuntime&,
        PrepareAction,NativeBaseObjectMemory&);
    // Reaction animation-wait and preparation children borrow real564/396.
    NativeCombatFatalChildren(ICombatActionTaskRuntime&,PrepareAction);
    // Flee reuses animation/text children plus real BeginFlee/Fled/Failed.
    NativeCombatFatalChildren(ICombatStatusTaskRuntime&,ICombatActionTaskRuntime&);
    NativeCombatFatalChildren(ICombatScriptTaskRuntime&, ICombatStatusTaskRuntime&);
    NativeCombatFatalChildren(ICombatScriptTaskRuntime&, ICombatStatusTaskRuntime&,
                             ICombatActionTaskRuntime&);
    NativeCombatFatalChildren(ICombatScriptTaskRuntime&, ICombatStatusTaskRuntime&,
                             NativeCombatFatalVisualServices);
    ~NativeCombatFatalChildren();
    NativeCombatFatalChildren(const NativeCombatFatalChildren&)=delete;
    NativeCombatFatalChildren& operator=(const NativeCombatFatalChildren&)=delete;
    [[nodiscard]] std::uint32_t Create(std::uint32_t factory_vtable);
    // Unlinked children only; linked children retire through the GOM container.
    void Destroy(std::uint32_t child);
    // ChangeAction text is initialized before BeginWaiting (phase0), including
    // the original recipe's already-linked, not-yet-waiting child.
    void AssignAction(std::uint32_t child, std::string_view action);
    void BindFinalHit(NativeCombatFatalVisualServices, INativeCombatFinalHitMessage&);
    [[nodiscard]] bool FinalHitBound() const noexcept;
    void AssignFinalMessage(std::uint32_t child, std::string_view name);
    [[nodiscard]] std::size_t OwnedActionTexts() const noexcept;
    [[nodiscard]] IGomTaskBaseRuntime& BaseRuntime() noexcept;
    [[nodiscard]] IGomTaskContainerRuntime& ContainerRuntime() noexcept;
    [[nodiscard]] std::size_t Count() const noexcept;
    // Excludes a borrowed domain; its containing owner accounts for that memory.
    [[nodiscard]] std::size_t OwnedAllocations() const noexcept;
    [[nodiscard]] bool Owns(std::uint32_t) const noexcept;
    void ValidateIdle() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
