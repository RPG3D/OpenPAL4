#include "pal4/combat_console_ui_initializer.h"

namespace pal4 {

std::uint8_t CombatConsoleUiInitializer::Initialize(
    ICombatConsoleUiRuntime& runtime) {
    const auto memory = runtime.Allocate(0x44u);
    const auto element = memory == 0u ? 0u :
        runtime.ConstructFontElement(memory);
    runtime.SetDimensions(element, 400.0f, 280.0f);
    runtime.SetImageset(element, "zi0", "zhangdoushengli", 1.0f);
    runtime.SetFont(element, "", "system", 1.0f);
    runtime.SetFontProperties(element, 2.0f, 3.0f);
    runtime.AddElement(runtime.ElementList(), element != 0u, element);
    return 1u;
}

}  // namespace pal4
