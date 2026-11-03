#pragma once
#include "pal4/strip_atomic_object.h"
namespace pal4 {
// SDK boundary for the game's strip mesh builder and callback policy.
// Owns an additional material reference consumed by shared Destroy.
class NativeStripAtomicRuntime final {
public:
    void Create(StripAtomicState&,std::uint32_t texture,const std::vector<StripPoint>&,
                float height,bool flip_uv,bool blend);
    void Destroy(StripAtomicState&);
};
}
