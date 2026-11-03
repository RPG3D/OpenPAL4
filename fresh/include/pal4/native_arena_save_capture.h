#pragma once
#include "pal4/native_arena_cache_save_section.h"
#include <string_view>

namespace pal4 {
class NativeSceneGobAssets;
class NativeMonsterManagerOwner;
class NativeMinimapManagerOwner;
class NativeArenaSaveCapture final {
public:
    // IDA51DFD0 boundary: capture the current creating owners in original
    // GOB -> Monster -> Minimap order; returned XML and names own their data.
    [[nodiscard]] static NativeArenaCacheEntry Capture(std::string_view world,
        std::string_view scene,NativeSceneGobAssets&,NativeMonsterManagerOwner&,
        NativeMinimapManagerOwner&);
};
} // namespace pal4
