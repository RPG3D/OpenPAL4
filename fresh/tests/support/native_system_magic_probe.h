#pragma once
#include <functional>
namespace pal4 {
class NativeSystemMenuPages;class NativeCeguiUiSession;class NativePalGameIVTextureStorage;
class NativeBaseObjectMemory;class NativeInventorySession;class NativeGameDatabase;
class NativeCommonDialogLayout;class NativeEventHandlerArray;struct PlayerManagerObjectState;
namespace test {
void VerifyNativeSystemMagicFactory(NativeSystemMenuPages&,NativeCeguiUiSession&,
    NativePalGameIVTextureStorage&,PlayerManagerObjectState&,NativeBaseObjectMemory&,
    NativeInventorySession&,const NativeGameDatabase&,NativeCommonDialogLayout&,
    NativeEventHandlerArray&,unsigned& created,unsigned& learning_sounds,
    const std::function<void()>& render,const std::function<unsigned(unsigned)>& key);
}
}
