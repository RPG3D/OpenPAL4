#include "pal4/system_toolbar_operations.h"
#include "pal4/ui_navigation_list.h"

#include <array>

namespace pal4 {
namespace {

struct Button {
    std::string_view name;
    std::uint32_t event_key;
    std::uint32_t frame_offset;
    bool forging;
};

constexpr std::array<Button, 7> kButtons{{
    {"sysToolBar/Property", 0x8a80bcu, 64u, false},
    {"sysToolBar/Magic", 0x8a81d0u, 68u, false},
    {"sysToolBar/Role", 0x8b00d4u, 72u, false},
    {"sysToolBar/Equipment", 0x8a8098u, 76u, false},
    {"sysToolBar/Task", 0x8a8548u, 80u, false},
    {"sysToolBar/System", 0x8b06a8u, 84u, false},
    {"sysToolBar/Forging", 0x8b0f1cu, 88u, true},
}};

}  // namespace

void SystemToolbarOperations::ClearAllSelection(
    const std::uint32_t frame, ISystemToolbarOperationsRuntime& runtime) {
    for (const auto& button : kButtons) {
        runtime.DispatchEvent(runtime.FrameManager() + 260u,
                              button.event_key, 0u, 0u);
        runtime.SetSelected(runtime.ReadWord(frame + button.frame_offset),
                            false);
    }
}

std::uint8_t SystemToolbarOperations::OnButtonClick(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemToolbarOperationsRuntime& runtime) {
    const auto source = runtime.ReadWord(event + 8u);
    const auto root = runtime.ReadWord(frame + 36u);
    const auto event_context = runtime.ReadWord(root + 68u);
    ClearAllSelection(frame, runtime);
    for (const auto& button : kButtons) {
        if (!runtime.CompareWindowName(source + 892u, button.name)) {
            continue;
        }
        if (button.forging) {
            runtime.WriteByte(runtime.SmithSystem(), 1u);
        }
        runtime.DispatchEvent(runtime.FrameManager() + 260u,
                              button.event_key, 1u, event_context);
        const auto window = runtime.ReadWord(frame + button.frame_offset);
        runtime.SetSelected(window, true);
        runtime.SetCurrentSelected(frame + 40u,
                                   runtime.ReadWord(frame + button.frame_offset));
        break;
    }
    return 1u;
}

std::uint8_t SystemToolbarOperations::Close(
    const std::uint32_t frame, ISystemToolbarOperationsRuntime& runtime) {
    ClearAllSelection(frame, runtime);
    runtime.SetVisible(frame, 0u, 0u);
    // IDA483438/483448 use +18C (current state), not +188 (state-list head).
    const auto scene = runtime.ReadWord(runtime.Game() + 0x18cu);
    if (scene != 0u) {
        const auto current_scene = runtime.ReadWord(runtime.Game() + 0x18cu);
        if (runtime.SceneMode(current_scene) == 5) {
            return runtime.ClearTradeItemInfo(runtime.TradeSystem());
        }
    }
    return runtime.InitializeGameState(runtime.Game(), 1u, 0u);
}

std::uint8_t SystemToolbarOperations::HandleKeyInput(
    const std::uint32_t member, const std::uint32_t event,
    ISystemToolbarOperationsRuntime& runtime) {
    const auto key = runtime.ReadWord(event + 16u);
    constexpr std::array<std::uint32_t, 7> offsets{
        32u, 24u, 36u, 28u, 48u, 40u, 44u};
    if (key < 59u || key > 65u) {
        return 0u;
    }
    runtime.HandleMouseEvent(runtime.ReadWord(member + offsets[key - 59u]));
    return 1u;
}

std::uint8_t SystemToolbarOperations::HandleActiveKeyDown(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemToolbarOperationsRuntime& runtime, IUiNavigationInputRuntime& navigation) {
    if (runtime.ReadWord(event + 16u) == 1u) {
        (void)Close(frame, runtime);
        return 1u;
    }
    return UiNavigationList::HandleKey(frame + 40u, event, navigation);
}

}  // namespace pal4
