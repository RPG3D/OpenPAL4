#include "pal4/crt_placement_new.h"

#include <new>

namespace pal4 {

std::uint32_t CrtPlacementNew::At(
    const std::uint32_t bytes,
    const std::uint32_t destination) noexcept {
    const auto storage = reinterpret_cast<void*>(static_cast<std::uintptr_t>(destination));
    return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(
        ::operator new(bytes, storage)));
}

}  // namespace pal4
