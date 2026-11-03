#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {
class NativeAnimatedActorAsset;
class NativeBaseObjectMemory;

// Connects borrowed actor records to the existing direct animation switch.
[[nodiscard]] bool SelectNativeBorrowedActorAnimation(
    std::uint32_t object, NativeAnimatedActorAsset& actor,
    NativeBaseObjectMemory& memory, std::string_view action);
}  // namespace pal4
