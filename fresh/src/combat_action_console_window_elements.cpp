#include "pal4/combat_action_console_window_elements.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::array<std::string_view, 6> kButtons{
    "BtnAttack", "BtnDefend", "BtnStunt",
    "BtnMagic", "BtnArticle", "BtnFlee"};

std::uint32_t Prefix(const std::uint32_t window,
                     ICombatActionConsoleWindowElementRuntime& runtime) {
    const auto prefix = runtime.ReadWord(window + 0x18u);
    return prefix != 0u ? prefix : runtime.EmptyText();
}

}  // namespace

void ICombatActionConsoleWindowElementRuntime::ReleaseConnection(std::uint32_t connection) {
    const auto remaining=ReadWord(connection+4u)-1u;
    WriteWord(connection+4u,remaining);
    if(!remaining) DestroyConnection(connection,1u);
}

std::uint32_t CombatActionConsoleWindowElements::Setup(
    const std::uint32_t window,
    ICombatActionConsoleWindowElementRuntime& runtime) {
    runtime.InitializeText();
    try {
        for (std::uint32_t index = 0u; index < kButtons.size(); ++index) {
            const auto name = runtime.FormatName(Prefix(window, runtime),
                                                 kButtons[index]);
            const auto button = runtime.FindWindowByName(
                window, runtime.ToUtf8(name));
            runtime.WriteWord(window + 0x150u + 4u * index, button);
            runtime.WriteWord(button + 0x118u, index);
            const auto callback = runtime.Allocate(0x18u);
            if (callback != 0u) {
                runtime.WriteWord(callback + 0x10u, window);
                runtime.WriteWord(callback + 8u, kClickCallback);
                runtime.WriteWord(callback + 0xcu, 0u);
                runtime.WriteWord(callback, kCallbackVtable);
            }
            const auto connection = runtime.SubscribeClicked(button, callback);
            if (connection != 0u) runtime.ReleaseConnection(connection);
        }
        const auto panel_name = runtime.FormatName(
            Prefix(window, runtime), "StaticControlPanel");
        const auto panel = runtime.FindWindow(window, panel_name);
        runtime.WriteWord(window + 0x1e0u, panel);
        runtime.SetAlwaysOnTop(panel, true);
        return runtime.CleanupText();
    } catch (...) {
        (void)runtime.CleanupText();
        throw;
    }
}

}  // namespace pal4
