#pragma once
#include <cstdint>
#include <string_view>
namespace pal4 {
struct NativeCombatSystemImage;class NativeMilesSound2dRuntime;
// Original5761B0 ->579ED0: independent temporary handle, with Combat mode gate.
// NativeUiSoundPlayback's separate4AC280 shared-handle suppression does not apply.
std::uint32_t PlayNativeCombatSound(NativeCombatSystemImage&,NativeMilesSound2dRuntime&,std::string_view,std::uint32_t mode);
}
