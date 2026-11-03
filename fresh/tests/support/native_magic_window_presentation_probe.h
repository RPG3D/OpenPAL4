#pragma once
#include <filesystem>
namespace pal4 {
class NativeCeguiUiSession;class NativeGameDatabase;
namespace test {
struct NativeFrameSession;
void VerifyNativeMagicWindowPresentation(NativeCeguiUiSession&,const std::filesystem::path&,
    const NativeGameDatabase&,NativeFrameSession&);
void VerifyNativeMagicWindowInteraction(NativeCeguiUiSession&,const std::filesystem::path&,
    const NativeGameDatabase&,NativeFrameSession&);
void VerifyNativeMagicWindowPointAction(NativeCeguiUiSession&,const std::filesystem::path&,
    const NativeGameDatabase&,NativeFrameSession&);
void VerifyNativeMagicWindowCastAction(NativeCeguiUiSession&,const std::filesystem::path&,
    const NativeGameDatabase&,NativeFrameSession&,bool stunt_mode=false);
}
}
