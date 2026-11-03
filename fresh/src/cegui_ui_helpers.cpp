#include "pal4/cegui_ui_helpers.h"

namespace pal4 {

std::uint16_t CeguiUiHelpers::ConvertImeCharacter(
    const std::uint8_t first_byte, const std::uint16_t initial_wide,
    ICeguiImeRuntime& runtime) {
    const std::array<std::uint8_t, 2> bytes{
        first_byte, static_cast<std::uint8_t>(initial_wide)};
    auto output = initial_wide;
    static_cast<void>(runtime.MultiByteToWideChar(
        0u, 1u, bytes, output, 1u));
    return output;
}

std::uint32_t CeguiUiHelpers::MapVirtualKey(
    const std::uint32_t owner, const std::uint32_t virtual_key,
    ICeguiKeyLookupRuntime& runtime) {
    const auto map = owner + 4u;
    const auto candidate = runtime.SearchNode(map, virtual_key);
    const auto sentinel = runtime.ReadDword(owner + 8u);
    const auto selected =
        candidate == sentinel ||
                virtual_key < runtime.ReadDword(candidate + 0x0cu)
            ? sentinel
            : candidate;
    if (selected == sentinel) return 0xffu;
    return runtime.ReadDword(selected + 0x10u);
}

}  // namespace pal4
