#include "pal4/trade_item_finder.h"

#include <string>
#include <vector>

namespace pal4 {

std::uint8_t TradeItemFinder::FindByTextAndId(
    const std::uint32_t text_address, const std::uint32_t id,
    ITradeItemFinderRuntime& runtime) {
    static_cast<void>(runtime.GameDatabaseInstance());
    const std::string first_copy{runtime.ReadText(text_address)};
    const std::string comparison{first_copy};
    const auto database = runtime.GameDatabaseInstance();
    const auto data = runtime.TradeData(database + kDatabaseTableOffset);
    const auto count = runtime.RecordCount(data + kCountFieldOffset);
    const auto base = runtime.RecordBase(data + kBaseFieldOffset);

    std::vector<std::uint32_t> matching_records;
    for (std::uint32_t i = 0u; i < count; ++i) {
        const auto record = base + i * kRecordBytes;
        if (runtime.ReadText(record + kRecordTextOffset) == comparison)
            matching_records.push_back(record);
    }
    for (const auto record : matching_records)
        if (runtime.ReadWord(record + kRecordIdOffset) == id) return 1u;
    return 0u;
}

}  // namespace pal4
