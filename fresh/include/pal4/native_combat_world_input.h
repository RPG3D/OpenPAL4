#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
struct NativeCombatSystemImage;class NativeCombatWorldResources;class NativeCombatTargetSelection;
class NativeCombatRosterTargets;class NativeCombatMouseTargets;class NativeScriptEventRouter;class InputManager;class WindowMessageConfig;
// Original53F720: world UV schemas then actual53BA50 input/periodic tips.
// Borrowed controller/rosters/camera/input/router must outlive synchronous calls.
class NativeCombatWorldInput final {
public:
    NativeCombatWorldInput(NativeCombatSystemImage&,NativeCombatWorldResources&,NativeCombatTargetSelection&,
        NativeCombatRosterTargets&,NativeCombatMouseTargets&,NativeScriptEventRouter&,InputManager&,WindowMessageConfig&);
    ~NativeCombatWorldInput();
    std::uint32_t CombatSystemAddress() const noexcept;
    std::uint32_t Update(float elapsed);
    bool WorldUpdated() const noexcept;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
