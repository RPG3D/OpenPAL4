#include "pal4/combat_console.h"

#include <string>
#include <string_view>

namespace pal4 {

std::uint32_t CombatConsoleWindow::InitializeMainWindow(
    const std::uint32_t console, ICombatConsoleMainWindowRuntime& runtime) {
    runtime.WriteWord(console + 212u,
                      runtime.FindWindowByName(
                          "CombatMainWindow/StaticActionSequence"));
    runtime.WriteWord(console + 216u,
                      runtime.FindWindow(
                          console, "CombatMainWindow/ImgJointAttackHint"));
    runtime.WriteWord(console + 220u,
                      runtime.FindWindow(
                          console, "CombatMainWindow/guaishuliang"));
    runtime.WriteWord(console + 260u,
                      runtime.FindWindow(
                          console, "CombatMainWindow/PanelMonsterInfo"));
    const auto edit = runtime.FindWindow(
        console, "CombatMainWindow/EditMonsterInfo");
    runtime.WriteWord(console + 264u, edit);
    runtime.SetAutoBreak(edit, false);

    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto name = runtime.ConvertToUtf8(
            "CombatMainWindow/StaticRole" + std::to_string(index + 1u));
        const auto image = runtime.FindWindowByName(name);
        runtime.WriteWord(console + 224u + index * 4u, image);
        runtime.SetChildVisible(image, false);
        runtime.WriteWord(image + 280u, 0u);
    }
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        const auto name = runtime.ConvertToUtf8(
            "CombatMainWindow/StaticMonster" + std::to_string(index + 1u));
        const auto image = runtime.FindWindowByName(name);
        runtime.WriteWord(console + 236u + index * 4u, image);
        runtime.SetChildVisible(image, false);
        runtime.WriteWord(image + 280u, 0u);
    }

    auto callback = runtime.Allocate(24u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, kClickCallback);
        runtime.WriteWord(callback + 12u, 0u);
        runtime.WriteWord(callback + 16u, console);
        runtime.WriteWord(callback, kClickCallbackVtable);
    }
    runtime.SubscribeClick(runtime.ReadWord(console + 36u),
                           runtime.MouseClickEvent(), callback);
    if (callback != 0u) runtime.ReleaseCallback(callback);
    return 0u;
}

}  // namespace pal4
