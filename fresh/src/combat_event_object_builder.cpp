#include "pal4/combat_event_object_builder.h"

#include <bit>

namespace pal4 {
namespace {

void WriteInvalid(std::uint32_t record,
                  ICombatEventObjectBuildRuntime& runtime) {
    runtime.WriteWord(record, 0u);
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 2u);
}

void WriteScaled(std::uint32_t record, std::uint32_t event,
                 double value, float scale,
                 ICombatEventObjectBuildRuntime& runtime) {
    runtime.WriteWord(record + 8u, 0u);
    runtime.WriteWord(record, event);
    runtime.WriteWord(record + 4u, std::bit_cast<std::uint32_t>(
        static_cast<float>(value * static_cast<double>(scale))));
}

}  // namespace

std::uint32_t CombatEventObjectBuilder::Build(
    std::uint32_t record, std::uint32_t actor,
    std::uint32_t index, float scale, std::uint32_t mode,
    ICombatEventObjectBuildRuntime& runtime) {
    const auto event = CombatEventDataRegistry::Lookup(index, runtime);
    if (event == 0u) {
        WriteInvalid(record, runtime);
        return record;
    }

    switch (mode) {
    case 0u:
        runtime.WriteWord(record + 8u, 0u);
        runtime.WriteWord(record, event);
        runtime.WriteWord(record + 4u, std::bit_cast<std::uint32_t>(scale));
        break;
    case 1u:
    case 3u: {
        const auto address = actor + (mode == 1u ? 0x154u : 4u);
        const auto value = runtime.ReadEventValue(event, address);
        WriteScaled(record, event, value, scale, runtime);
        break;
    }
    case 2u:
        runtime.WriteWord(record, event);
        runtime.WriteWord(record + 4u, std::bit_cast<std::uint32_t>(scale));
        runtime.WriteWord(record + 8u, 1u);
        break;
    case 4u:
        if (index == 6u || index == 2u) {
            const auto address = actor + (index == 6u ? 0x160u : 0x158u);
            const auto word = runtime.ReadWord(address);
            const auto value = std::bit_cast<std::int32_t>(word);
            WriteScaled(record, event, static_cast<double>(value), scale,
                        runtime);
        } else {
            WriteInvalid(record, runtime);
        }
        break;
    default:
        WriteInvalid(record, runtime);
        break;
    }
    return record;
}

}  // namespace pal4
