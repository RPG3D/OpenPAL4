#include "pal4/game_data_equipment_attribute_codec.h"

namespace pal4 {

std::uint32_t GameDataEquipmentAttributeCodec::Deserialize(
    std::uint32_t object, std::uint32_t stream,
    IGameDataEquipmentAttributeCodecRuntime& runtime) {
    (void)runtime.ReadRaw(stream, object + 0x90, 4);
    (void)runtime.ReadRaw(stream, object + 0x94, 4);
    for (std::uint32_t index = 0; index < 13; ++index)
        runtime.ReadText(stream, object + 0x98 + index * 16);
    for (std::uint32_t offset = 0x168; offset < 0x190; offset += 4)
        (void)runtime.ReadRaw(stream, object + offset, 4);
    return runtime.ReadRaw(stream, object, 0x90);
}

std::uint32_t GameDataEquipmentAttributeCodec::Serialize(
    std::uint32_t object, std::uint32_t stream,
    IGameDataEquipmentAttributeCodecRuntime& runtime) {
    (void)runtime.WriteRaw(stream, object + 0x90, 4);
    (void)runtime.WriteRaw(stream, object + 0x94, 4);
    for (std::uint32_t index = 0; index < 13; ++index)
        runtime.WriteText(stream, object + 0x98 + index * 16);
    for (std::uint32_t offset = 0x168; offset < 0x190; offset += 4)
        (void)runtime.WriteRaw(stream, object + offset, 4);
    return runtime.WriteRaw(stream, object, 0x90);
}

}  // namespace pal4
