#include "pal4/combat_result_icon_creator.h"

#include <array>
#include <bit>

namespace pal4 {

std::uint8_t CombatResultIconCreator::Create(
    const std::uint32_t element,
    ICombatResultIconRuntime& runtime) {
    runtime.PlaySound("WA44.wav", 1u);
    const auto dialog = runtime.ReadWord(element + 60u);
    const auto score = runtime.ReadWord(dialog + 428u);
    constexpr std::array<const char*, 4> images{
        "tedeng2", "yideng2", "erdeng2", "sandeng2"};
    if (score >= images.size()) return 1u;

    const auto icon = runtime.Allocate(0x48u);
    if (icon != 0u) {
        (void)runtime.ConstructFontElement(icon);
        runtime.WriteWord(icon, kVtable);
    }
    runtime.SetDimensions(icon, 450.0f, 430.0f);
    runtime.SetImageset(icon, "zi0", images[score], 20.0f);
    runtime.WriteWord(icon + 68u, std::bit_cast<std::uint32_t>(60.0f));
    runtime.SetFont(icon, "", "system", 1.0f);
    runtime.SetFontProperties(icon, 2.0f, 3.0f);
    runtime.AddElement(runtime.ElementList(), true, icon);
    return 1u;
}

}  // namespace pal4
