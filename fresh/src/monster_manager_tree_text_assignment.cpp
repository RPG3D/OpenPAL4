#include "pal4/monster_manager_tree_text_assignment.h"

namespace pal4 {

void MonsterManagerTreeTextAssignment::Assign(
    std::uint32_t destination, std::uint32_t source,
    IMonsterManagerTreeTextRuntime& runtime) {
    if (destination == 0u) return;

    const auto mode = runtime.ReadByte(source);
    runtime.WriteDword(destination + 4u, 0u);
    runtime.WriteByte(destination, mode);
    runtime.WriteDword(destination + 8u, 0u);
    runtime.WriteDword(destination + 0x0cu, 0u);
    const auto length = runtime.ReadDword(source + 8u);

    if (destination == source) {
        runtime.DetachAndReset(destination);
        runtime.DetachAndReset(destination);
    } else {
        auto source_data = runtime.ReadDword(source + 4u);
        if (source_data == 0u) source_data = runtime.EmptyTextData();
        if (length != 0u && runtime.ReadByte(source_data - 1u) < 0xfeu) {
            runtime.SetErrorFlag(destination, 1u);
            runtime.WriteDword(destination + 4u, source_data);
            runtime.WriteDword(destination + 8u, length);
            runtime.WriteDword(destination + 0x0cu,
                               runtime.ReadDword(source + 0x0cu));
            const auto count = runtime.ReadByte(source_data - 1u);
            runtime.WriteByte(source_data - 1u,
                              static_cast<std::uint8_t>(count + 1u));
        } else if (runtime.AllocateText(destination, length, 1u)) {
            runtime.CopyBytes(runtime.ReadDword(destination + 4u),
                              source_data, length);
            runtime.WriteDword(destination + 8u, length);
            runtime.WriteByte(runtime.ReadDword(destination + 4u) + length,
                              0u);
        }
    }
    runtime.WriteDword(destination + 0x10u,
                       runtime.ReadDword(source + 0x10u));
}

}  // namespace pal4
