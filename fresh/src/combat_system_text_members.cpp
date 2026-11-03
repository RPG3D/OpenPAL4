#include "pal4/combat_system_text_members.h"

namespace pal4 {

std::uint32_t CombatSystemTextMembers::ReleaseAt(
    const std::uint32_t address, ICombatSystemTextRuntime& runtime) {
    const auto data = runtime.ReadWord(address);
    std::uint32_t result = 0u;
    if (data != 0u) {
        const auto header = data - 1u;
        const auto references = runtime.ReadByte(header);
        if (references == 0u || references == 0xffu)
            result = runtime.Free(header);
        else {
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(references - 1u));
            result = header;
        }
    }
    runtime.WriteWord(address, 0u);
    runtime.WriteWord(address + 4u, 0u);
    runtime.WriteWord(address + 8u, 0u);
    return result;
}

std::uint32_t CombatSystemTextMembers::Cleanup(
    const std::uint32_t object, ICombatSystemTextRuntime& runtime) {
    static_cast<void>(ReleaseAt(object + 56u, runtime));
    static_cast<void>(ReleaseAt(object + 40u, runtime));
    return ReleaseAt(object + 24u, runtime);
}

}  // namespace pal4
