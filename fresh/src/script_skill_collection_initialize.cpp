#include "pal4/script_skill_collection.h"

#include "pal4/game_data_object_lifecycle.h"

#include <array>

namespace pal4 {

std::uint32_t ScriptSkillCollection::Initialize(
    std::uint32_t object, IScriptSkillCollectionInitRuntime& runtime) {
    static_cast<void>(GameDataObjectLifecycle::ConstructBaseStates(
        object, runtime));
    const auto high_byte = static_cast<std::uint8_t>(object >> 24u);
    runtime.WriteByte(object + 676u, high_byte);
    for (const auto offset : std::array{680u, 684u, 688u})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteByte(object + 692u, high_byte);
    for (const auto offset : std::array{696u, 700u, 704u})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteWord(object, kVtable);
    return object;
}

}  // namespace pal4
