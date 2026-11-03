#include "pal4/game_info_window.h"
#include "pal4/preview_text_codec.h"

#include <array>
#include <string>
#include <utility>

namespace pal4 {

namespace {

void WriteCoreText(const std::uint32_t object,
                   const std::string& formatted,
                   IGameInfoWindowRuntime& runtime) {
    runtime.SetText(runtime.ReadWord(object + 0x2cu),
                    PreviewTextCodec::DecodeUtf8(formatted));
    for (const auto [trigger, offset] :
         {std::pair{0x63u, 0x30u}, std::pair{0x64u, 0x38u}}) {
        const auto text = runtime.TriggerText(trigger);
        const auto utf8 = runtime.ConvertToUtf8(text);
        runtime.SetText(runtime.ReadWord(object + offset),
                        PreviewTextCodec::DecodeUtf8(utf8));
    }
}

}  // namespace

std::uint32_t GameInfoWindow::Construct(
    const std::uint32_t object, const std::uint32_t layout_name,
    IGameInfoWindowRuntime& runtime) {
    runtime.LoadLayout(object, layout_name);
    try {
        runtime.InitializeText(object + 0x40u,
                               static_cast<std::uint8_t>(layout_name));
        try {
            runtime.AssignText(object + 0x40u, "updateTime");
            runtime.WriteWord(object, kVtable);
            (void)InitializeControls(object, runtime);
        } catch (...) {
            runtime.DestroyText(object + 0x40u);
            throw;
        }
    } catch (...) {
        (void)runtime.DestroyBase(object);
        throw;
    }
    return object;
}

std::uint32_t GameInfoWindow::Destroy(
    const std::uint32_t object, IGameInfoWindowRuntime& runtime) {
    runtime.DestroyText(object + 0x40u);
    return runtime.DestroyBase(object);
}

std::uint32_t GameInfoWindow::InitializeControls(
    const std::uint32_t object, IGameInfoWindowRuntime& runtime) {
    constexpr std::array<std::string_view, 6> kNames{
        "btnHelp", "staticMoney", "staticTime", "staticTimeInfo",
        "staticGameTime", "staticGameTimeInfo"};
    const std::string prefix(runtime.WindowName(object));
    try {
        for (std::uint32_t index = 0; index < kNames.size(); ++index) {
            runtime.WriteWord(object + 0x28u + index * 4u,
                              runtime.GetWindow(prefix + std::string(kNames[index])));
        }
        const auto callback = runtime.Allocate(0xcu);
        if (callback != 0u) {
            runtime.WriteWord(callback + 4u, 0x4a6b60u);
            runtime.WriteWord(callback + 8u, object);
            runtime.WriteWord(callback, kHelpCallbackVtable);
        }
        const auto connection = runtime.SubscribeClicked(
            runtime.ReadWord(object + 0x28u), callback);
        if (connection != 0u) runtime.ReleaseConnection(connection);
    } catch (...) {
        (void)runtime.ReleaseNameBuffer();
        throw;
    }
    return runtime.ReleaseNameBuffer();
}

std::uint8_t GameInfoWindow::OnHelpButtonClick(
    IGameInfoWindowRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    const auto help_id = runtime.ReadWord(manager + 0x144u);
    runtime.ProcessUiEvent(runtime.FrameManager() + 0x104u,
                           kHelpEventKey, {1u, help_id});
    return 1u;
}

bool GameInfoWindow::HandleShow(
    const std::uint32_t event, IGameInfoWindowRuntime& runtime) {
    return runtime.HandleShow(kNamePointer, event);
}

std::uint8_t GameInfoWindow::SetHelpTipsId(
    const std::uint32_t event, IGameInfoWindowRuntime& runtime) {
    (void)runtime.FindFrame(runtime.FrameManager(), kNamePointer);
    const auto manager = runtime.FrameManager();
    runtime.WriteWord(manager + 0x144u, runtime.ReadWord(event));
    return 1u;
}

bool GameInfoWindow::HandleUpdate(IGameInfoWindowRuntime& runtime) {
    const auto frame = runtime.FindFrame(runtime.FrameManager(), kNamePointer);
    if (frame != 0u) (void)UpdateUi(frame, runtime);
    return true;
}

std::uint32_t GameInfoWindow::UpdateUi(
    const std::uint32_t object, IGameInfoWindowRuntime& runtime) {
    const std::string formatted = std::to_string(runtime.InventoryMoney());
    try {
        WriteCoreText(object, formatted, runtime);
    } catch (...) {
        (void)runtime.ReleaseFormatted(formatted);
        throw;
    }
    return runtime.ReleaseFormatted(formatted);
}

std::uint8_t GameInfoWindow::SetVisible(
    const std::uint32_t object, const bool visible,
    const std::uint32_t parent, IGameInfoWindowRuntime& runtime) {
    runtime.SetParent(object, visible, parent);
    if (!visible) return 0u;
    const std::string formatted = std::to_string(runtime.InventoryMoney());
    try {
        WriteCoreText(object, formatted, runtime);
        (void)UpdateClock(object, runtime);
    } catch (...) {
        (void)runtime.ReleaseFormatted(formatted);
        throw;
    }
    return static_cast<std::uint8_t>(runtime.ReleaseFormatted(formatted));
}

std::uint8_t GameInfoWindow::UpdateClock(
    const std::uint32_t object, IGameInfoWindowRuntime& runtime) {
    if (!runtime.IsVisible(object)) return 0u;
    const auto elapsed = runtime.ElapsedSeconds();
    const auto total = runtime.FormatSeconds(
        elapsed + runtime.ClockOffsetSeconds());
    try {
        const auto session = runtime.FormatSeconds(elapsed);
        try {
            runtime.SetText(runtime.ReadWord(object + 0x34u),
                            PreviewTextCodec::DecodeUtf8(total));
            runtime.SetText(runtime.ReadWord(object + 0x3cu),
                            PreviewTextCodec::DecodeUtf8(session));
        } catch (...) {
            (void)runtime.ReleaseTimeString(session);
            throw;
        }
        (void)runtime.ReleaseTimeString(session);
    } catch (...) {
        (void)runtime.ReleaseTimeString(total);
        throw;
    }
    return runtime.ReleaseTimeString(total);
}

}  // namespace pal4
