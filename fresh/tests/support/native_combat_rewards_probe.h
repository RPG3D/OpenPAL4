#pragma once
namespace pal4 {
class NativeCombatConsoleVisuals;class NativeCombatConsole;class NativeMilesSound2dRuntime;class NativeCombatEndingWindow;class NativeCombatReport;struct NativeCombatSystemImage;
class NativePalGameIVTextureStorage;class NativeCombatEventHandlers;class NativePlayerManagerSession;
class NativeSkillRecordCatalog;class INativeSkillSystemServices;class NativePlayerEventRuntime;
class NativeUpgradeRecordCatalog;class NativeCombatFormulaCatalog;class NativeGameDatabase;
namespace test {
void VerifyNativeCombatRewards(const NativeCombatReport&,NativeCombatSystemImage&,
    NativePalGameIVTextureStorage&,NativeCombatEventHandlers&,NativePlayerManagerSession&,
    NativeSkillRecordCatalog&,INativeSkillSystemServices&,NativePlayerEventRuntime&,
    const NativeUpgradeRecordCatalog&,const NativeCombatFormulaCatalog&,const NativeGameDatabase&,NativeCombatEndingWindow&,NativeMilesSound2dRuntime&,NativeCombatConsoleVisuals&,NativeCombatConsole&);
}
}
