#pragma once
#include "pal4/native_arena_cache_save_section.h"
#include <optional>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeMonsterManagerOwner;
class NativeMonsterStorageCapture final {
public:
    [[nodiscard]] static std::optional<NativeArenaCacheChild> Capture(
        NativeBaseObjectMemory& memory, std::uint32_t object, const BinaryXmlConfig& config);
    [[nodiscard]] static std::vector<NativeArenaCacheChild> CaptureScene(
        NativeMonsterManagerOwner& monsters);
};
}
