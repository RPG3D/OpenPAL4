#pragma once
namespace pal4 {
class NativeCeguiUiSession;class NativeGameDatabase;class NativeBaseObjectMemory;
class NativeMagicWindowRecords;class NativeMagicWindowLayout;class NativeInventorySession;
class NativeRoleSelector;class NativePlayerSkillRuntime;struct PlayerManagerObjectState;
class NativeEventHandlerArray;
namespace test {
struct NativeFrameSession;
void VerifyNativeStuntActions(NativeCeguiUiSession&,NativeFrameSession&,const NativeGameDatabase&,
    PlayerManagerObjectState&,NativeBaseObjectMemory&,const NativeMagicWindowRecords&,
    NativeMagicWindowLayout&,NativeInventorySession&,NativeRoleSelector&,NativePlayerSkillRuntime&,NativeEventHandlerArray&);
}
}
