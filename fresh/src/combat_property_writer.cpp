#include "pal4/combat_property_writer.h"

namespace pal4 {

std::uint32_t CombatPropertyWriter::Key(
    const std::uint32_t record, ICombatPropertyWriterRuntime& runtime) {
    const auto data = runtime.ReadWord(record + 4u);
    return data == 0u ? kEmptyText : data;
}

std::uint32_t CombatPropertyWriter::SetCombatProperties(
    const std::uint32_t node, const std::uint32_t scene,
    const std::uint32_t area, const std::uint32_t start_type,
    const std::uint32_t property110, ICombatPropertyWriterRuntime& runtime) {
    runtime.SetString(node, Key(0x8fa264u, runtime), scene);
    runtime.SetString(node, Key(0x8fa254u, runtime), area);
    static_cast<void>(runtime.WriteInt(
        node, Key(0x8fa244u, runtime), start_type));
    return runtime.SetProperty110(runtime.CombatSystemInstance(),
                                  property110);
}

std::uint8_t CombatPropertyWriter::SetMonsterProperties(
    const std::uint32_t document, const std::uint32_t monster_id,
    const std::uint32_t position, const std::uint32_t abnormal_state,
    ICombatPropertyWriterRuntime& runtime) {
    const auto node = runtime.CreateNode(document,
                                         Key(0x8fa224u, runtime));
    static_cast<void>(runtime.WriteInt(
        node, Key(0x8fa214u, runtime), monster_id));
    static_cast<void>(runtime.WriteInt(
        node, Key(0x8fa200u, runtime), position));
    return runtime.WriteInt(node, Key(0x8fa1f0u, runtime),
                            abnormal_state);
}

}  // namespace pal4
