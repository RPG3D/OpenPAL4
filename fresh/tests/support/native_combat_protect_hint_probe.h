#pragma once
namespace pal4 {
class NativeCombatHints;class NativeGameTimerQueue;class NativeGameDatabase;class NativeCeguiUiSession;class NativeMilesSound2dRuntime;class NativeInventorySession;
namespace test {void VerifyNativeCombatMoneyHints(NativeCombatHints&,NativeCeguiUiSession&,NativeGameTimerQueue&,const NativeGameDatabase&,NativeMilesSound2dRuntime&);void VerifyNativeCombatMonsterFleeHints(NativeCombatHints&,NativeCeguiUiSession&,NativeGameTimerQueue&,const NativeGameDatabase&);void VerifyNativeCombatProtectHint(NativeCombatHints&,NativeGameTimerQueue&,const NativeGameDatabase&);}
namespace test {void VerifyNativeCombatAcquisitionHints(NativeCombatHints&,NativeCeguiUiSession&,NativeGameTimerQueue&,const NativeGameDatabase&,const NativeInventorySession&,NativeMilesSound2dRuntime&);}
}
