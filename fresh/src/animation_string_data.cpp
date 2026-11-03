#include "pal4/animation_string_file_reader.h"
#include "pal4/animation_string_table_collection.h"

#include <algorithm>
#include <limits>

namespace pal4 {
namespace {

bool ReadWord(std::span<const std::uint8_t>& bytes, std::uint32_t& value) {
    if (bytes.size() < 4u) return false;
    value = static_cast<std::uint32_t>(bytes[0]) |
            (static_cast<std::uint32_t>(bytes[1]) << 8u) |
            (static_cast<std::uint32_t>(bytes[2]) << 16u) |
            (static_cast<std::uint32_t>(bytes[3]) << 24u);
    bytes = bytes.subspan(4u);
    return true;
}

}  // namespace

bool AnimationStringFileReader::Load(
    std::string& text, std::span<const std::uint8_t>& bytes) {
    std::uint32_t length{};
    if (!ReadWord(bytes, length) ||
        length > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) ||
        length > bytes.size()) return false;
    const auto payload = bytes.first(length);
    const auto end = std::find(payload.begin(), payload.end(), 0u);
    if (payload.empty()) text.clear();
    else text.assign(reinterpret_cast<const char*>(payload.data()),
                     static_cast<std::size_t>(end - payload.begin()));
    bytes = bytes.subspan(length);
    return true;
}

bool AnimationStringTable::LoadFromBytes(
    AnimationStringTableData& table, std::span<const std::uint8_t>& bytes) {
    table = {};
    AnimationStringTableData loaded;
    std::uint32_t count{};
    if (!AnimationStringFileReader::Load(loaded.name, bytes) ||
        !ReadWord(bytes, count) || count > bytes.size() / 8u) return false;
    for (std::uint32_t index = 0u; index < count; ++index) {
        std::string name, value;
        if (!AnimationStringFileReader::Load(name, bytes) ||
            !AnimationStringFileReader::Load(value, bytes)) return false;
        loaded.entries.emplace_back(std::move(name), std::move(value));
    }
    table = std::move(loaded);
    return true;
}

const std::string* AnimationStringTable::FindString(
    const AnimationStringTableData& table, const std::string_view name) {
    for (const auto& entry : table.entries)
        if (entry.first == name) return &entry.second;
    return nullptr;
}

bool AnimationStringTableCollection::LoadFromBytes(
    std::vector<AnimationStringTableData>& tables,
    std::span<const std::uint8_t> bytes) {
    tables.clear();
    std::uint32_t magic{}, count{};
    if (!ReadWord(bytes, magic) || magic != 20070323u ||
        !ReadWord(bytes, count) || count > bytes.size() / 8u) return false;
    std::vector<AnimationStringTableData> loaded;
    for (std::uint32_t index = 0u; index < count; ++index) {
        AnimationStringTableData table;
        if (!AnimationStringTable::LoadFromBytes(table, bytes)) return false;
        loaded.push_back(std::move(table));
    }
    tables = std::move(loaded);
    return true;
}

const AnimationStringTableData* AnimationStringTableCollection::Find(
    const std::span<const AnimationStringTableData> tables,
    const std::string_view name) {
    for (const auto& table : tables)
        if (table.name == name) return &table;
    return nullptr;
}

}  // namespace pal4
