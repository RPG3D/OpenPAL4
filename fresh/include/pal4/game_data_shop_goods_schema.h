#pragma once

#include "pal4/game_data_field_schema.h"

namespace pal4 {

class GameDataShopGoodsSchema final {
public:
    // IDA 0x4eeae0: register the six shop goods fields in binding order.
    [[nodiscard]] static std::uint8_t RegisterFields(
        std::uint32_t binding, IGameDataFieldSchemaRuntime& runtime);
};

}  // namespace pal4
