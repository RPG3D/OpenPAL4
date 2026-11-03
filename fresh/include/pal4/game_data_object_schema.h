#pragma once

#include "pal4/game_data_field_schema.h"

namespace pal4 {

class GameDataObjectSchema final {
public:
    // IDA 0x4ee840: register the four fields of a 56-byte object record.
    [[nodiscard]] static std::uint8_t RegisterFields(
        std::uint32_t binding, IGameDataFieldSchemaRuntime& runtime);
};

}  // namespace pal4
