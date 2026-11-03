#include "pal4/game_data_shop_goods_schema.h"

namespace pal4 {

std::uint8_t GameDataShopGoodsSchema::RegisterFields(
    std::uint32_t binding, IGameDataFieldSchemaRuntime& runtime) {
    runtime.RegisterField(binding, "ID", 0, GameDataFieldKind::integer);
    runtime.RegisterField(binding, "SHOP_ID", 20, GameDataFieldKind::text);
    runtime.RegisterField(binding, "GOODS_ID", 8,
                          GameDataFieldKind::integer);
    runtime.RegisterField(binding, "GOODS_TYPE", 4,
                          GameDataFieldKind::integer);
    runtime.RegisterField(binding, "OPEN_CONDITION", 12,
                          GameDataFieldKind::integer);
    runtime.RegisterField(binding, "OPEN_NUM", 16,
                          GameDataFieldKind::integer);
    return 1;
}

}  // namespace pal4
