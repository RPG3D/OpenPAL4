#pragma once

#include "pal4/arena_cache_entry.h"

#include <cstdint>

namespace pal4 {

class ArenaCache final {
public:
    static constexpr std::uint32_t kEntrySize = 72u;
    static constexpr std::uint32_t kObjectSize = 20u;
    static constexpr std::uint32_t kSingletonId = 1051u;

    // IDA 0x51e500: guarded registration for palGameArenaSaveCacheMgr.
    [[nodiscard]] static std::uint32_t Register(
        std::uint8_t& guard, std::uint32_t& registered,
        IArenaCacheEntryRuntime& runtime);
    // IDA 0x51e540: singleton factory.
    [[nodiscard]] static std::uint32_t Create(IArenaCacheEntryRuntime& runtime);
    // IDA 0x51e5a0: singleton deletion callback.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t cache, IArenaCacheEntryRuntime& runtime);
    // IDA 0x51e5c0: initialize a 20-byte manager.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t cache, IArenaCacheEntryRuntime& runtime);
    // IDA 0x51e5e0: destroy all 72-byte entries and release their array.
    [[nodiscard]] static std::uint32_t ReleaseEntries(
        std::uint32_t cache, IArenaCacheEntryRuntime& runtime);
    // IDA 0x51e630: clear child snapshots then destroy active entries.
    [[nodiscard]] static std::uint32_t ClearEntries(
        std::uint32_t cache, IArenaCacheEntryRuntime& runtime);
    // IDA 0x51e6d0: capture into a matching entry or insert a new one.
    [[nodiscard]] static std::uint32_t CaptureOrInsert(
        std::uint32_t cache, std::uint32_t first_name,
        std::uint32_t second_name, std::uint32_t temporary,
        IArenaCacheEntryRuntime& runtime);
    // IDA 0x51e820: restore a named entry to the live arena.
    [[nodiscard]] static std::uint32_t Restore(
        std::uint32_t cache, std::uint32_t first_name,
        std::uint32_t second_name, IArenaCacheEntryRuntime& runtime);
    // IDA 0x51e8c0: capture then serialize every active entry.
    [[nodiscard]] static std::uint32_t Save(
        std::uint32_t cache, std::uint32_t first_name,
        std::uint32_t second_name, std::uint32_t data,
        std::uint32_t temporary, IArenaCacheEntryRuntime& runtime);
    // IDA 0x51e940: clear entries, read count, insert then load each entry.
    [[nodiscard]] static std::uint32_t Load(
        std::uint32_t cache, std::uint32_t data,
        std::uint32_t temporary, IArenaCacheEntryRuntime& runtime);
    // IDA 0x51ea70: insert copies of one 72-byte entry at an array position.
    [[nodiscard]] static std::uint32_t InsertEntries(
        std::uint32_t cache, std::uint32_t position, std::uint32_t count,
        std::uint32_t source, IArenaCacheEntryRuntime& runtime);
};

}  // namespace pal4
