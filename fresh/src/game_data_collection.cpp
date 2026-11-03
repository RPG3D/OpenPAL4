#include "pal4/game_data_collection.h"

#include <vector>

namespace pal4 {

bool GameDataCollection::CheckValueRange(
    const GameDataRange range, const std::uint32_t record,
    IGameDataCollectionRuntime& runtime) {
    const auto address = record + 56u +
                         static_cast<std::uint32_t>(range.index) * 4u;
    const auto value = static_cast<std::int32_t>(runtime.ReadWord(address));
    return value > 0 && value <= range.maximum;
}

std::uint32_t GameDataCollection::BuildNewMatches(
    const std::uint32_t set, const std::uint32_t output,
    const GameDataRange range, const std::uint32_t seed_address,
    IGameDataCollectionRuntime& runtime) {
    const auto owner = runtime.GameDataOwner();
    std::vector<std::uint32_t> matching;
    std::uint32_t offset = 0u;
    for (std::uint32_t index = 0u; index < runtime.RecordCount(owner);
         ++index, offset += 208u) {
        const auto record = runtime.ReadWord(owner + 0x60u) + offset;
        if (CheckValueRange(range, record, runtime)) {
            matching.push_back(record);
        }
    }

    std::vector<std::uint32_t> new_records;
    for (const auto record : matching) {
        if (!runtime.ContainsId(set, runtime.ReadWord(record))) {
            new_records.push_back(record);
        }
    }
    for (const auto record : matching) {
        runtime.InsertUniqueId(set, runtime.ReadWord(record));
    }

    runtime.WriteByte(output, static_cast<std::uint8_t>(seed_address));
    const auto allocated = runtime.AllocateMemory(
        static_cast<std::uint32_t>(new_records.size()) * 4u);
    runtime.WriteWord(output + 4u, allocated);
    auto cursor = allocated;
    for (const auto record : new_records) {
        if (allocated != 0u) runtime.WriteWord(cursor, record);
        cursor += 4u;
    }
    runtime.WriteWord(output + 8u, cursor);
    runtime.WriteWord(output + 12u, cursor);
    return output;
}

}  // namespace pal4
