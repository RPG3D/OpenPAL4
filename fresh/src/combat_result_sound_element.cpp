#include "pal4/combat_result_sound_element.h"

namespace pal4 {

std::uint8_t CombatResultSoundElement::Show(
    const std::uint32_t element,
    ICombatResultSoundRuntime& runtime) {
    if (runtime.ReadByte(element + 65u) != 0u)
        runtime.PlaySound("WA46.wav", 1u);
    const auto visible = runtime.ReadByte(element + 64u) != 0u;
    const auto window = runtime.ReadWord(element + 60u);
    runtime.SetVisible(window, visible);
    return 1u;
}

}  // namespace pal4
