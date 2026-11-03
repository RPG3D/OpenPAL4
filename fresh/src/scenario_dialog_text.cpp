#include "pal4/scenario_dialog.h"

#include <cstdint>
#include <string>

namespace pal4 {

std::uint32_t ScenarioDialog::SetText(
    const std::uint32_t object, const std::uint32_t source,
    const std::uint32_t editbox_value,
    IScenarioTextRuntime& runtime) {
    const auto editbox = runtime.ReadWord(object + 56u);
    runtime.WriteWord(editbox + 0x878u, editbox_value);
    const auto text_pointer = runtime.ReadWord(source + 4u);
    const auto text = text_pointer != 0u
        ? runtime.ReadCString(text_pointer) : std::string{};
    const auto utf8 = runtime.ConvertToUtf8(text);
    runtime.SetUtf8Text(editbox, utf8);
    runtime.Show(object, 1u, 0u);
    const auto manager = runtime.UiFrameManager();
    return runtime.InsertHandler(manager + 0x134u, object + 40u);
}

}  // namespace pal4
