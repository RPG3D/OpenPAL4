#include "pal4/game_data_shop_goods_record_text.h"

namespace pal4 {

std::uint32_t GameDataShopGoodsRecordText::Release(
    std::uint32_t record,
    IGameDataShopGoodsRecordTextRuntime& runtime) {
    const auto data = runtime.ReadWord(record + 24);
    auto result = data;
    if (data) {
        const auto header = data - 1;
        const auto references = runtime.ReadByte(header);
        result = header;
        if (references == 0 || references == 0xff)
            result = runtime.Free(header);
        else
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(references - 1));
    }
    runtime.WriteWord(record + 24, 0);
    runtime.WriteWord(record + 28, 0);
    runtime.WriteWord(record + 32, 0);
    return result;
}

}  // namespace pal4
