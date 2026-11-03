#include "pal4/combat_event_object_record.h"

namespace pal4 {

std::uint32_t CombatEventObjectRecord::Process(
    std::uint32_t record, std::uint32_t target,
    ICombatEventObjectRecordRuntime& runtime) {
    const auto event = runtime.ReadWord(record);
    if (event == 0u) return record;
    const auto mode = runtime.ReadWord(record + 8u);
    if (mode == 0u)
        return runtime.ApplyModeZero(event, target,
                                     runtime.ReadWord(record + 4u));
    if (mode == 1u)
        return runtime.ApplyModeOne(event, target,
                                    runtime.ReadWord(record + 4u));
    return record;
}

std::uint8_t CombatEventObjectRecord::Query(
    std::uint32_t record, std::uint32_t argument,
    ICombatEventObjectRecordRuntime& runtime) {
    const auto event = runtime.ReadWord(record);
    if (event == 0u) return 0u;
    return static_cast<std::uint8_t>(
        runtime.Query(event, record + 4u, argument));
}

}  // namespace pal4
