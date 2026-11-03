#include "pal4/pal_actor_control_game_event.h"

#include <limits>
#include <stdexcept>

namespace pal4 {

std::int32_t IActorControlGameEventRuntime::EventCode(
    std::uint32_t) { return 0; }
std::uint32_t IActorControlGameEventRuntime::CreateHintWindow() {
    return 0u;
}
std::optional<std::string> IActorControlGameEventRuntime::TriggerText(
    std::uint32_t) { return std::nullopt; }
std::string IActorControlGameEventRuntime::ConvertToUtf8(
    std::string_view text) { return std::string(text); }
std::string IActorControlGameEventRuntime::AssignHintText(
    std::string_view utf8) { return std::string(utf8); }
std::uint32_t IActorControlGameEventRuntime::EncodedSize(
    std::string_view utf8) {
    return static_cast<std::uint32_t>(utf8.size());
}
std::string IActorControlGameEventRuntime::EncodeHintText(
    std::string_view utf8, std::uint32_t) {
    return std::string(utf8);
}
void IActorControlGameEventRuntime::InitializeHintWindow(
    std::uint32_t, std::string_view, float, float, float,
    std::uint32_t, std::uint32_t) {}
void IActorControlGameEventRuntime::PlayHintSound(
    std::string_view, std::uint32_t) {}

bool PalActorControlGameEvent::Process(
    PalActorControlState& state, std::uint32_t dialog,
    IActorControlGameEventRuntime& runtime) {
    const auto event = runtime.EventCode(dialog);
    if (event < 0 || state.words[19] ==
                         static_cast<std::uint32_t>(event))
        return true;
    if (event != 1 && event != 3) return true;

    const auto window = runtime.CreateHintWindow();
    if (window == 0u) return false;
    const auto raw = runtime.TriggerText(event == 1 ? 0x109u : 0x10au);
    auto raw_text = raw.value_or("");
    const auto raw_terminator = raw_text.find('\0');
    if (raw_terminator != std::string::npos)
        raw_text.resize(raw_terminator);
    auto utf8 = runtime.ConvertToUtf8(raw_text);
    const auto terminator = utf8.find('\0');
    if (terminator != std::string::npos) utf8.resize(terminator);
    std::string hint;
    if (event == 1) {
        hint = runtime.AssignHintText(utf8);
    } else {
        const auto encoded_size = runtime.EncodedSize(utf8);
        if (encoded_size == std::numeric_limits<std::uint32_t>::max())
            throw std::length_error("length for UTF8 encoding");
        hint = runtime.EncodeHintText(utf8, encoded_size);
    }
    runtime.InitializeHintWindow(window, hint, 0.5f, 0.15f,
                                 1.0f, 0u, 1u);
    runtime.PlayHintSound(kSoundPath, 1u);
    return false;
}

}  // namespace pal4
