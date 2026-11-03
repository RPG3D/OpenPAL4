#include "pal4/system_dialog_visibility.h"

namespace pal4 {

void SystemDialogVisibility::Update(
    const std::uint32_t frame, const std::uint8_t visible,
    const std::string_view variable_name, const std::uint32_t parent,
    ISystemDialogVisibilityRuntime& runtime) {
    if (visible != 0u) {
        const auto variable = runtime.TriggerVariable(
            runtime.GameDatabase() + 52u, variable_name);
        const std::string source(runtime.TriggerText(variable));
        const auto utf8 = runtime.ConvertToUtf8(source);
        const auto nul = utf8.find('\0');
        const auto length = nul == std::string::npos ? utf8.size() : nul;
        const auto cegui_string = runtime.CreateCeguiString(
            std::string_view(utf8.data(), length));
        try {
            runtime.SetText(runtime.ReadWord(frame + 40u), cegui_string);
        } catch (...) {
            runtime.DestroyCeguiString(cegui_string);
            throw;
        }
        runtime.DestroyCeguiString(cegui_string);
    }
    runtime.SetParent(frame, visible, parent);
    const auto root = runtime.ReadWord(frame + 36u);
    runtime.SetAlwaysOnTop(root, true);
    runtime.SetModalState(runtime.ReadWord(frame + 36u), visible);
}

}  // namespace pal4
