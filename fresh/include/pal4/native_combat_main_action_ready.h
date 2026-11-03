#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeCombatMainWindow;
class NativeCombatMainUiEvents;
class NativeCombatConsole;
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCeguiUiSession;
class NativeCameraManagerOwner;
class NativeCombatConsoleVisuals;
class NativeEffectSceneSession;
class NativeGameTimerQueue;
//54D670 real portrait, effect and timer consumer. Borrows every dependency;
// Close before Main, console, actors, effect session and UI retirement.
class NativeCombatMainActionReady final {
public:
    NativeCombatMainActionReady(NativeCombatMainWindow&,NativeCombatMainUiEvents&,
        NativeCombatConsole&,NativeCombatPlayerManager&,NativeCombatMonsterManager&,
        NativeCeguiUiSession&,NativeCameraManagerOwner&,NativeCombatConsoleVisuals&,
        NativeEffectSceneSession&,NativeGameTimerQueue&);
    ~NativeCombatMainActionReady();
    void Handle(std::uint32_t payload);
    bool HandleTimer(std::uint32_t owner,std::uint32_t method);
    void Close();
    std::size_t OutstandingAllocations() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
