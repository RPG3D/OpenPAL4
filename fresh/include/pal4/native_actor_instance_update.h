#pragma once
#include <cstdint>
namespace pal4 {
class NativeAnimatedActorAsset;
class NativeActorAnimationStateBinding;
void InitializeNativeActorInstanceBounds(std::uint32_t,const NativeAnimatedActorAsset&);
std::uint32_t UpdateNativeActorInstance(std::uint32_t,float,const NativeAnimatedActorAsset&,
                                      NativeActorAnimationStateBinding&);
}
