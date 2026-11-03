#pragma once
struct RpClump;
struct RpLight;
namespace pal4 {
// The light/frame are owned; the shadow clump is borrowed and outlives them.
[[nodiscard]] RpLight* CreateNativeActorShadowLight(RpClump* shadow);
void DestroyNativeActorShadowLight(RpLight* light) noexcept;
}
