#pragma once
#include <cstdint>
#include <span>
#include <string_view>
struct RpClump;
namespace pal4 {
//52A340/52A3A0/52A3F0/52A490 over borrowed real component and SDK clumps.
// The caller retains both clumps and must remove the child's attachment before
// either is retired. This neither owns resources nor installs a fake parent.
class NativeActorAnimationBinding final {
public:
    static std::uint32_t Update(std::span<std::uint32_t,116> component,
        RpClump& child,RpClump& parent,std::string_view hook,std::uint32_t mode);
};
}
