#pragma once

#include "pal4/arena_cache.h"
#include "pal4/native_arena_cache_save_section.h"
#include "pal4/native_base_object_memory.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pal4 {
class NativeGameSaveArchiveReader;

// Owns the original 20-byte arena cache manager for the native x86 runtime.
class NativeArenaCacheOwner final : private IArenaCacheEntryRuntime {
public:
    NativeArenaCacheOwner();
    NativeArenaCacheOwner(const NativeArenaCacheOwner&) = delete;
    NativeArenaCacheOwner& operator=(const NativeArenaCacheOwner&) = delete;
    ~NativeArenaCacheOwner() override;

    [[nodiscard]] std::uint32_t Address() const noexcept { return cache_; }
    [[nodiscard]] std::uint32_t Restore(std::string_view arena,
                                        std::string_view area);
    // IDA518A7A..93: retain the departing scene without writing an archive.
    void CaptureCurrent(const NativeArenaCacheEntry& entry);
    [[nodiscard]] bool SaveCurrent(const NativeArenaCacheEntry& entry,
                                   NativeGameSaveArchiveBuffer& archive);
    [[nodiscard]] bool LoadSection(std::span<const std::uint8_t> bytes,
                                   std::size_t& consumed);
    [[nodiscard]] bool LoadSection(NativeGameSaveArchiveReader& archive);
    void BindRestoreCallbacks(
        std::function<std::uint32_t(std::string_view)> find_game_object,
        std::function<void(std::uint32_t, const NativeArenaCacheChild&)>
            restore_game_object,
        std::function<void(std::uint32_t,
                           const std::vector<std::vector<std::uint32_t>>&)> restore_minimap,
        std::function<std::uint32_t(std::string_view)> find_monster = {},
        std::function<void(std::uint32_t, const NativeArenaCacheChild&)> restore_monster = {});
    [[nodiscard]] std::uint32_t LastLogEvent() const noexcept {
        return last_log_event_;
    }
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }
    // IDA51E630: retire snapshots while retaining the cache owner.
    void ClearEntries();
    void Destroy();

private:
    void ProcessCurrent(const NativeArenaCacheEntry& entry,
                        NativeGameSaveArchiveBuffer* archive);
    struct CachedMinimap {
        std::uint32_t header = 0u;
        std::vector<std::vector<std::uint32_t>> elements;
    };
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    [[nodiscard]] std::uint32_t Free(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t bytes) override;
    [[nodiscard]] bool NamesEqualIgnoreCase(std::uint32_t left,
                                             std::uint32_t right) override;
    [[nodiscard]] std::uint32_t Call(
        ArenaCacheEntryCall operation,
        std::initializer_list<std::uint32_t> arguments) override;

    NativeBaseObjectMemory memory_;
    const NativeArenaCacheEntry* current_entry_ = nullptr;
    NativeGameSaveArchiveBuffer* current_archive_ = nullptr;
    std::span<const std::uint8_t> read_bytes_;
    std::size_t read_offset_ = 0u;
    std::unordered_map<std::uint32_t, NativeArenaCacheChild> children_;
    std::unordered_map<std::uint32_t, CachedMinimap> minimaps_;
    std::function<std::uint32_t(std::string_view)> find_game_object_;
    std::function<void(std::uint32_t, const NativeArenaCacheChild&)>
        restore_game_object_;
    std::function<void(std::uint32_t,
                       const std::vector<std::vector<std::uint32_t>>&)> restore_minimap_;
    std::uint32_t cache_ = 0u;
    std::function<std::uint32_t(std::string_view)> find_monster_;
    std::function<void(std::uint32_t, const NativeArenaCacheChild&)> restore_monster_;
    std::uint32_t last_log_event_ = 0u;
};

}  // namespace pal4
