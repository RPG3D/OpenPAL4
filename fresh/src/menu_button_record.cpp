#include "pal4/menu_button_record.h"

namespace pal4 {

std::uint32_t MenuButtonRecord::CopyConstruct(
    const std::uint32_t destination, const std::uint32_t source,
    IMenuButtonRecordDataRuntime& runtime) {
    if (destination == 0u) return 0u;

    for (std::uint32_t offset = 0u; offset != 12u; offset += 4u)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));

    runtime.WriteWord(destination + 16u, kInlineCapacity);
    runtime.WriteWord(destination + 20u, 0u);
    runtime.WriteWord(destination + 28u, 0u);
    runtime.WriteWord(destination + 24u, 0u);
    runtime.WriteWord(destination + 12u, 0u);
    runtime.WriteWord(destination + 32u, 0u);

    const auto length = runtime.ReadWord(source + 12u);
    runtime.GrowString(destination + 12u, length);
    const auto destination_capacity = runtime.ReadWord(destination + 16u);
    runtime.WriteWord(destination + 12u, length);
    const auto destination_data = destination_capacity <= kInlineCapacity
        ? destination + 32u : runtime.ReadWord(destination + 160u);
    runtime.WriteWord(destination_data + length * 4u, 0u);

    const auto source_data = runtime.ReadWord(source + 16u) <= kInlineCapacity
        ? source + 32u : runtime.ReadWord(source + 160u);
    for (std::uint32_t index = 0u; index < length; ++index)
        runtime.WriteWord(destination_data + index * 4u,
                          runtime.ReadWord(source_data + index * 4u));
    return length * 4u;
}

std::uint32_t MenuButtonRecord::Assign(
    const std::uint32_t destination, const std::uint32_t source,
    IMenuButtonRecordDataRuntime& runtime) {
    for (std::uint32_t offset = 0u; offset != 12u; offset += 4u)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    runtime.AssignString(destination + 12u, source + 12u, 0u, kNpos);
    return destination;
}

}  // namespace pal4
