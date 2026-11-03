#pragma once

#include "pal4/binary_xml_config.h"
#include "pal4/native_game_save_archive_buffer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace pal4 {

struct NativeArenaCacheChild {
    // Negative signed IDs occupy only the tag and carry no document on disk.
    // Native ArenaCacheEntry::Load skips them; structured parsing retains them.
    std::uint32_t id = 0u;
    BinaryXmlConfig document;
};

struct NativeArenaCacheEntry {
    std::array<std::string, 2u> names;
    std::array<std::vector<NativeArenaCacheChild>, 2u> groups;
    std::uint32_t minimap_header = 0u;
    std::vector<std::vector<std::uint32_t>> minimap_elements;
};

struct NativeArenaCacheSection {
    std::vector<NativeArenaCacheEntry> entries;
};

class NativeArenaCacheSaveSection final {
public:
    explicit NativeArenaCacheSaveSection(NativeGameSaveArchiveBuffer& archive)
        : archive_(archive) {}

    [[nodiscard]] bool Serialize(const NativeArenaCacheSection& section);
    [[nodiscard]] static std::optional<NativeArenaCacheSection> ParsePrefix(
        std::span<const std::uint8_t> bytes, std::size_t& consumed);

private:
    NativeGameSaveArchiveBuffer& archive_;
};

}  // namespace pal4
