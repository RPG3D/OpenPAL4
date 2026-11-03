#include "pal4/script_hint_display.h"

#include <limits>
#include <stdexcept>

namespace pal4 {

void ScriptHintDisplay::Show(
    std::uint32_t text_payload, float x, float y,
    IScriptHintDisplayRuntime& runtime) {
    auto source = runtime.ReadWord(text_payload + 4u);
    if (source == 0u) source = runtime.EmptyStringAddress();
    auto utf8 = runtime.ConvertToUtf8(source);
    const auto terminator = utf8.find('\0');
    if (terminator != std::string::npos) utf8.resize(terminator);
    const auto size = runtime.EncodedSize(utf8);
    if (size == std::numeric_limits<std::uint32_t>::max())
        throw std::length_error("Length for utf8 encoded string can not be npos");
    const auto text = runtime.Encode(utf8, size);
    const auto window = runtime.CreateHintWindow(runtime.UiFrameManager());
    runtime.ShowHint(window, text, x, y, 2.5f, 0u, 0u);
}

}  // namespace pal4
