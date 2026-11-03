#pragma once
struct RpClump;
namespace pal4 {
class NativeHookClumpNames;
// Reuses 5FCF80 before shadow rendering and 5FD040 afterwards.
void ApplyNativeShadowAtomicFlags(RpClump* clump, NativeHookClumpNames& names, bool restore);
}
