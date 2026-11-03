#include "pal4/native_arena_cache_owner.h"
#include "pal4/native_game_save_archive_reader.h"
#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

pal4::NativeArenaCacheChild Child(const std::uint32_t id,
                                 const std::string& name) {
    pal4::BinaryXmlNode root;
    root.name = "soGOBEffect";
    root.attributes.push_back({"valName", 3u, name});
    const auto bytes = pal4::BinaryXmlConfig::Serialize(root);
    pal4::NativeArenaCacheChild child;
    child.id = id;
    Check(bytes && child.document.Parse(*bytes),
          "construct synthetic cache child XML");
    return child;
}

pal4::NativeArenaCacheSection Parse(
    const pal4::NativeGameSaveArchiveBuffer& archive) {
    const auto& bytes = archive.Bytes();
    std::size_t consumed = 0u;
    auto section = pal4::NativeArenaCacheSaveSection::ParsePrefix(
        std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size()),
        consumed);
    Check(section && consumed == bytes.size(),
          "native cache save parses its complete output");
    return std::move(*section);
}

void CheckArchiveInput(const pal4::NativeGameSaveArchiveBuffer& saved,
                       const pal4::NativeArenaCacheEntry& second) {
    const auto root = std::filesystem::path("H:/PAL4/objs/arena-cache-input") /
        std::to_string(GetCurrentProcessId());
    std::filesystem::create_directories(root / "save");
    auto fixture = [&](std::span<const std::byte> bytes) {
        std::ofstream file(root / "save/0.dat", std::ios::binary);
        file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        Check(static_cast<bool>(file), "write continuous cache fixture");
    };
    pal4::NativeGameSaveArchiveBuffer stream;
    Check(stream.WriteMarker("PREFIX") && stream.WriteBytes(saved.Bytes()) &&
              stream.WriteMarker("ARENA_SERIALIZE_FLAG") && stream.WriteConfig(pal4::BinaryXmlNode{"ARENA"}),
          "compose cache followed by arena config");
    fixture(stream.Bytes());
    pal4::NativeGameSaveArchiveReader archive;
    Check(archive.OpenSlot(root, 0) && archive.ReadMarker() == "PREFIX", "open prefix fixture");
    const auto start = archive.Position();
    Check(!archive.ReadPrefix([](auto, auto&) { return false; }) && archive.Position() == start,
          "parser rejection preserves nonzero cursor");
    Check(!archive.ReadPrefix([](auto, auto& consumed) { consumed = 0u; return true; }) && archive.Position() == start,
          "zero-length parser success is rejected");
    Check(!archive.ReadPrefix([](auto bytes, auto& consumed) { consumed = bytes.size() + 1u; return true; }) &&
              archive.Position() == start, "oversized parser result cannot move beyond file");
    bool failed = false;
    try {
        (void)archive.ReadPrefix([](auto, auto&) -> bool { throw std::runtime_error("test prefix exception"); });
    } catch (const std::runtime_error&) { failed = true; }
    Check(failed && archive.Position() == start, "parser exception propagates and restores cursor");
    pal4::NativeArenaCacheOwner cache;
    Check(cache.LoadSection(archive) && archive.Position() == start + saved.Bytes().size(),
          "cache owner reads only its prefix from continuous archive");
    Check(archive.ReadMarker() == "ARENA_SERIALIZE_FLAG", "next arena marker remains unread by cache");
    pal4::BinaryXmlConfig config;
    Check(archive.ReadConfig(config) && config.Root().name == "ARENA" && archive.Remaining() == 0u,
          "shared XML prefix path resumes after cache and marker");
    archive.Close();
    const auto allocations = cache.OutstandingAllocations();
    Check(archive.OpenSlot(root, 0) && archive.ReadMarker() == "PREFIX" && cache.LoadSection(archive) &&
              cache.OutstandingAllocations() == allocations, "repeat cache input replaces snapshots without allocation leaks");
    archive.Close();
    fixture(std::span(saved.Bytes()).first(saved.Bytes().size() - 1u));
    Check(archive.OpenSlot(root, 0) && !cache.LoadSection(archive) && archive.Position() == 0u &&
              archive.LastError() == "invalid save arena cache", "truncated cache rejects and restores archive cursor");
    archive.Close();
    pal4::NativeGameSaveArchiveBuffer unchanged;
    Check(cache.SaveCurrent(second, unchanged) && unchanged.Bytes() == saved.Bytes(),
          "truncated stream preserves all previously loaded cache state");
    cache.Destroy();
    Check(cache.OutstandingAllocations() == 0u, "stream-loaded cache destroys all persistent buffers");
    std::filesystem::remove_all(root);
}

void CheckNegativeStorageIds() {
    pal4::NativeArenaCacheSection section;
    pal4::NativeArenaCacheEntry entry;
    entry.names = {"negative-id-world", "negative-id-area"};
    entry.groups[0] = {{0xffffffffu, {}}, {0x80000000u, {}}};
    entry.groups[1] = {{0xfffffff9u, {}}};
    section.entries.push_back(entry);
    pal4::NativeGameSaveArchiveBuffer bytes;
    pal4::NativeArenaCacheSaveSection writer(bytes);
    Check(writer.Serialize(section), "write negative storage tags without XML payload");
    const auto parsed = Parse(bytes);
    Check(parsed.entries.size() == 1u && parsed.entries[0].groups[0].size() == 2u &&
              parsed.entries[0].groups[0][1].id == 0x80000000u && parsed.entries[0].groups[1].size() == 1u,
          "prefix parser preserves skipped storage tags and exact minimap boundary");
    pal4::NativeGameSaveArchiveBuffer rebuilt;
    pal4::NativeArenaCacheSaveSection roundtrip(rebuilt);
    Check(roundtrip.Serialize(parsed) && rebuilt.Bytes() == bytes.Bytes(), "negative-tag structured roundtrip");
    pal4::NativeArenaCacheOwner cache;
    std::size_t consumed{};
    Check(cache.LoadSection(std::span(reinterpret_cast<const std::uint8_t*>(bytes.Bytes().data()), bytes.Bytes().size()), consumed) &&
              consumed == bytes.Bytes().size(), "native shared cache loader skips negative IDs in both groups");
    unsigned minimaps = 0u;
    cache.BindRestoreCallbacks([](auto) -> std::uint32_t { throw std::runtime_error("negative child must not be looked up"); },
        [](auto, const auto&) { throw std::runtime_error("negative child must not be restored"); },
        [&](auto header, const auto& elements) { Check(header == 0u && elements.empty(), "minimap after skipped tags"); ++minimaps; });
    Check(cache.Restore(entry.names[0], entry.names[1]) == 1u && minimaps == 1u,
          "negative tags create no restore children");
    cache.Destroy();
    Check(cache.OutstandingAllocations() == 0u, "negative tags leave no child ownership leaks");
}

}  // namespace

int main() {
    try {
        CheckNegativeStorageIds();
        {
            pal4::NativeArenaCacheOwner departing;
            pal4::NativeArenaCacheEntry room1, room2, room3;
            room1.names = {"M07", "1"};
            room2.names = {"M07", "2"};
            room3.names = {"M07", "3"};
            room1.groups[0].push_back(Child(8u, "door1"));
            room2.groups[0].push_back(Child(8u, "door2"));
            departing.CaptureCurrent(room1);
            departing.CaptureCurrent(room2);
            room1.groups[0].push_back(Child(8u, "opened-key"));
            departing.CaptureCurrent(room1);
            pal4::NativeGameSaveArchiveBuffer written;
            Check(departing.SaveCurrent(room3, written),
                  "save after visiting scenes without intermediate saves");
            pal4::NativeArenaCacheSection expected;
            expected.entries = {room1, room2, room3};
            pal4::NativeGameSaveArchiveBuffer expected_bytes;
            pal4::NativeArenaCacheSaveSection writer(expected_bytes);
            Check(writer.Serialize(expected) && written.Bytes() == expected_bytes.Bytes(),
                  "departing scenes persist in order and revisits replace without duplicates");
            CheckArchiveInput(written, room3);
            departing.Destroy();
            Check(departing.OutstandingAllocations() == 0u,
                  "capture-only visits release all snapshots and temporary projections");
        }
        pal4::NativeArenaCacheOwner cache;
        const auto address = cache.Address();
        const auto* words = reinterpret_cast<const std::uint32_t*>(
            static_cast<std::uintptr_t>(address));
        Check(address != 0u && cache.OutstandingAllocations() == 1u &&
                  words[0] == 1u && words[2] == 0u && words[3] == 0u &&
                  words[4] == 0u,
              "new-game arena cache starts with no saved entries");
        Check(cache.Restore("Q01", "N01") == 0u &&
                  cache.LastLogEvent() == 95u &&
                  cache.OutstandingAllocations() == 1u,
              "first Q01/N01 restore follows the original cache-miss branch");
        pal4::NativeArenaCacheEntry first;
        first.names = {"Q01", "N01"};
        first.groups[0].push_back(Child(8u, "effect001"));
        first.minimap_header = 0xffff0000u;
        pal4::NativeGameSaveArchiveBuffer first_archive;
        Check(cache.SaveCurrent(first, first_archive),
              "capture and save first arena cache entry");
        const auto first_saved = Parse(first_archive);
        Check(first_saved.entries.size() == 1u &&
                  first_saved.entries[0].names == first.names &&
                  first_saved.entries[0].groups[0].size() == 1u &&
                  first_saved.entries[0].groups[0][0].id == 8u &&
                  first_saved.entries[0].minimap_header == 0xffff0000u,
              "first cache entry retains names, child and minimap");
        first.groups[0].push_back(Child(8u, "effect002"));
        pal4::NativeGameSaveArchiveBuffer updated_archive;
        Check(cache.SaveCurrent(first, updated_archive),
              "recapture matching arena cache entry");
        const auto updated = Parse(updated_archive);
        Check(updated.entries.size() == 1u &&
                  updated.entries[0].groups[0].size() == 2u,
              "matching entry replaces its prior children");
        pal4::NativeArenaCacheEntry second;
        second.names = {"Q01", "Q01"};
        second.groups[0].push_back(Child(8u, "effect101"));
        second.minimap_header = 0x12340001u;
        second.minimap_elements = {{1u, 2u, 3u}};
        pal4::NativeGameSaveArchiveBuffer second_archive;
        Check(cache.SaveCurrent(second, second_archive),
              "capture and save a second arena cache entry");
        const auto second_saved = Parse(second_archive);
        Check(second_saved.entries.size() == 2u &&
                  second_saved.entries[0].names == first.names &&
                  second_saved.entries[0].groups[0].size() == 2u &&
                  second_saved.entries[1].names == second.names &&
                  second_saved.entries[1].groups[0].size() == 1u &&
                  second_saved.entries[1].minimap_header ==
                      second.minimap_header &&
                  second_saved.entries[1].minimap_elements ==
                      second.minimap_elements,
              "cache save writes retained entries in insertion order");
        pal4::NativeArenaCacheSection expected;
        expected.entries = {first, second};
        pal4::NativeGameSaveArchiveBuffer expected_archive;
        pal4::NativeArenaCacheSaveSection expected_writer(expected_archive);
        Check(expected_writer.Serialize(expected) &&
                  expected_archive.Bytes() == second_archive.Bytes(),
              "ArenaCache::Save matches structured bytes with minimap data");
        CheckArchiveInput(second_archive, second);
        pal4::NativeArenaCacheOwner loaded_cache;
        const auto& saved_bytes = second_archive.Bytes();
        const auto input = std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t*>(saved_bytes.data()),
            saved_bytes.size());
        std::size_t loaded_bytes = 0u;
        Check(loaded_cache.LoadSection(input, loaded_bytes) &&
                  loaded_bytes == input.size(),
              "ArenaCache::Load consumes two saved entries");
        std::vector<std::string> restored_names;
        std::vector<std::uint32_t> restored_minimaps;
        loaded_cache.BindRestoreCallbacks(
            [](const std::string_view name) {
                return name.starts_with("effect") ? 0x100u : 0u;
            },
            [&](const std::uint32_t object,
                const pal4::NativeArenaCacheChild& child) {
                Check(object == 0x100u,
                      "cache restore finds the matching live GOB");
                restored_names.push_back(std::get<std::string>(
                    child.document.Root().FindAttribute("valName")->value));
            },
            [&](const std::uint32_t header, const auto&) {
                restored_minimaps.push_back(header);
            });
        Check(loaded_cache.Restore("Q01", "N01") == 1u &&
                  loaded_cache.Restore("Q01", "Q01") == 1u &&
                  restored_names == std::vector<std::string>{
                      "effect001", "effect002", "effect101"} &&
                  restored_minimaps == std::vector<std::uint32_t>{
                      0xffff0000u, 0x12340001u},
              "ArenaCache::Restore applies named GOBs and minimaps in order");
        pal4::NativeGameSaveArchiveBuffer reloaded_archive;
        Check(loaded_cache.SaveCurrent(second, reloaded_archive) &&
                  reloaded_archive.Bytes() == second_archive.Bytes(),
              "loaded ArenaCache saves identical child and minimap bytes");
        std::size_t rejected_bytes = 1u;
        Check(!loaded_cache.LoadSection(input.first(input.size() - 1u),
                                        rejected_bytes) &&
                  rejected_bytes == 0u,
              "truncated ArenaCache input is rejected before mutation");
        pal4::NativeGameSaveArchiveBuffer after_rejection;
        Check(loaded_cache.SaveCurrent(second, after_rejection) &&
                  after_rejection.Bytes() == second_archive.Bytes(),
              "rejected input preserves previously loaded cache entries");
        const auto retained_owner = loaded_cache.Address();
        loaded_cache.ClearEntries();
        loaded_cache.ClearEntries();
        Check(loaded_cache.Address() == retained_owner &&
                  loaded_cache.Restore("Q01", "N01") == 0u &&
                  loaded_cache.Restore("Q01", "Q01") == 0u,
              "NewGame clears populated snapshots without destroying cache owner");
        Check(loaded_cache.LoadSection(input, loaded_bytes) &&
                  loaded_cache.Restore("Q01", "N01") == 1u,
              "cleared cache accepts original snapshots again");
        loaded_cache.Destroy();
        Check(loaded_cache.OutstandingAllocations() == 0u,
              "loaded ArenaCache releases its children and minimap");
        cache.Destroy();
        Check(cache.Address() == 0u &&
                  cache.OutstandingAllocations() == 0u,
              "arena cache manager releases its native allocation");
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: unexpected exception: %s\n", error.what());
        return EXIT_FAILURE;
    }
}
