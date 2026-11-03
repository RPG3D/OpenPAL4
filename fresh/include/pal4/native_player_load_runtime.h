#pragma once

#include <cstdint>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeGameSaveArchiveReader;
class NativePlayerSlotCopyRuntime;
class IPlayerManagerSlotCollectionRuntime;
class IPlayerManagerSlotScriptRuntime;
class IScriptProcessRuntime;

// Reads one complete PR section at the current archive cursor. Persistent
// buffers and Role strings belong to the supplied player session owners.
// All three post-load service groups are required; there are no default stubs.
class NativePlayerLoadRuntime final {
public:
    [[nodiscard]] static bool Deserialize(
        std::uint32_t player, NativeBaseObjectMemory& memory,
        NativePlayerSlotCopyRuntime& text_owner,
        NativeGameSaveArchiveReader& archive,
        IPlayerManagerSlotCollectionRuntime& collections,
        IPlayerManagerSlotScriptRuntime& refresh,
        IScriptProcessRuntime& scripts);
};
}  // namespace pal4
