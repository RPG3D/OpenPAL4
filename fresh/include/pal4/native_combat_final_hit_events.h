#pragma once

#include <cstdint>

namespace pal4 {
class IFinalHitTaskRuntime;
class NativeScriptEventRouter;

// The router and task runtime must outlive this subscription. Clear it before
// retiring the task or its effect handle.
class NativeCombatFinalHitEvents final {
public:
    NativeCombatFinalHitEvents(NativeScriptEventRouter&, IFinalHitTaskRuntime&,
                               std::uint32_t task);
    ~NativeCombatFinalHitEvents();
    NativeCombatFinalHitEvents(const NativeCombatFinalHitEvents&) = delete;
    NativeCombatFinalHitEvents& operator=(const NativeCombatFinalHitEvents&) = delete;
    void Subscribe();
    void Clear() noexcept;
    [[nodiscard]] bool Active() const noexcept;
private:
    NativeScriptEventRouter& router_;
    IFinalHitTaskRuntime& runtime_;
    std::uint32_t task_;
    std::uint32_t handle_{};
};
}
