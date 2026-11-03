#pragma once
#include "pal4/native_combat_ending_sequence.h"
namespace pal4 {
class NativeCombatEndingPresentation;class NativeMilesSound2dRuntime;class NativeCombatConsoleVisuals;
class NativeCombatEndingLevelUp;class NativeCombatEndSequence;
class NativeCombatConsole;
class NativeUiFrameUpdate;
// Binds the actual report presentation to547270, real upgrade/console services,
// and synchronous576440. All borrowed owners survive callbacks and Close.
class NativeCombatEndingSession final {
public:
    NativeCombatEndingSession(NativeCombatEndingWindow&,NativeCombatEndingPresentation&,
        NativeCombatPlayerManager&,NativeCombatTaskQueue&,NativeMilesSound2dRuntime&,
        NativeCombatSystemImage&,const NativeUpgradeRecordCatalog&,
        NativeCombatEndingSequence::Skills,NativeCombatEndingSequence::Actions,
        NativeCombatEndingSequence::Visuals,NativeCombatConsole&,NativeCombatConsoleVisuals&,
        NativeCombatEndingLevelUp&,NativeCombatEndSequence&,NativeUiFrameUpdate&);
    ~NativeCombatEndingSession();
    void Update(double seconds);
    // Report callbacks may run inside the battle queue. The next UI tick
    // composes the original sequence after borrowed action dispatch unwinds.
    bool PendingStart()const noexcept;
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
