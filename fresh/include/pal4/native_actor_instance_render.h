#pragma once
#include <cstdint>
namespace pal4 {
class NativeAnimatedActorAsset;
class NativeBaseObjectMemory;
// Borrow the owned model image/SDK graph during original52AC10 or667590.
// Skeleton debug drawing and populated shadow cameras still need real services.
void RenderNativeActorInstance(std::uint32_t actor,std::uint32_t camera,std::uint32_t slot,
    const NativeAnimatedActorAsset&,NativeBaseObjectMemory&);
}
