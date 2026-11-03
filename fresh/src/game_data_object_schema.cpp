#include "pal4/game_data_object_schema.h"

namespace pal4 {

std::uint8_t GameDataObjectSchema::RegisterFields(
    std::uint32_t binding, IGameDataFieldSchemaRuntime& runtime) {
    runtime.RegisterField(binding, "ID", 4, GameDataFieldKind::text);
    runtime.RegisterField(binding, "NAME", 20, GameDataFieldKind::text);
    runtime.RegisterField(binding, "POSITION", 36,
                          GameDataFieldKind::text);
    runtime.RegisterField(binding, "TYPE", 52,
                          GameDataFieldKind::integer);
    return 1;
}

}  // namespace pal4
