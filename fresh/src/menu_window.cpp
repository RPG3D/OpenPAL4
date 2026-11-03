#include "pal4/menu_window.h"

#include <string>

namespace pal4 {
namespace {

constexpr std::uint32_t kCloseFunction = 0x4b4b30u;
constexpr std::uint32_t kClickFunction = 0x4b4c00u;
constexpr std::uint32_t kKeyFunction = 0x484bd0u;

void SubscribeEvent(const std::uint32_t owner,
                    const std::uint32_t root,
                    const std::uint32_t function,
                    const std::string_view event_name,
                    IMenuWindowRuntime& runtime) {
    const auto callback = runtime.Allocate(24u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, function);
        runtime.WriteWord(callback + 12u, 0u);
        runtime.WriteWord(callback + 16u, owner);
        runtime.WriteWord(callback, MenuWindow::kEventCallbackVtable);
    }
    runtime.Subscribe(root, event_name, callback);
    if (callback != 0u)
        runtime.ReleaseSubscriptionCallback(callback);
}

}  // namespace

std::uint32_t MenuWindow::Construct(const std::uint32_t window,
                                    IMenuWindowRuntime& runtime) {
    runtime.LoadLayout(window, "menuWindow");
    runtime.ConstructNavigation(window + 40u);
    runtime.WriteWord(window + 40u, 0x843628u);
    runtime.WriteWord(window + 68u, 0u);
    runtime.WriteByte(window + 64u, runtime.InitialStackByte());
    runtime.WriteWord(window + 72u, 0u);
    runtime.WriteWord(window + 76u, 0u);
    runtime.WriteWord(window + 40u, kNavigationVtable);
    runtime.WriteWord(window, kVtable);

    const auto storage = runtime.Allocate(24u);
    const auto close_callback = storage == 0u
        ? 0u : runtime.ConstructCloseCallback(
                   storage, kCloseFunction, 0u, window);
    runtime.RegisterCloseEvent("close_popup_menu", close_callback);
    if (close_callback != 0u)
        runtime.ReleaseCloseCallback(close_callback);

    const std::string panel_name =
        std::string(runtime.ReadPrefix(window + 20u)) + "PanelMenu";
    runtime.WriteWord(window + 80u, runtime.FindWindow(panel_name));
    const std::string button_name =
        std::string(runtime.ReadPrefix(window + 20u)) + "BtnModel";
    const auto button = runtime.FindWindow(button_name);
    runtime.WriteWord(window + 84u, button);
    runtime.SetVisible(button, false);
    runtime.SetEnabled(button, false);

    const auto root = runtime.ReadWord(window + 36u);
    SubscribeEvent(window, root, kClickFunction,
                   "EventMouseClick", runtime);
    SubscribeEvent(window, root, kKeyFunction,
                   "EventKeyDown", runtime);
    return window;
}

std::uint32_t MenuWindow::Destruct(const std::uint32_t window,
                                   IMenuWindowRuntime& runtime) {
    runtime.WriteWord(window, kVtable);
    runtime.WriteWord(window + 40u, kNavigationVtable);
    const auto begin = runtime.ReadWord(window + 68u);
    const auto end = runtime.ReadWord(window + 72u);
    for (auto cursor = begin; cursor != end; cursor += 164u)
        runtime.DestroyString(cursor + 12u);
    runtime.Free(begin);
    runtime.WriteWord(window + 68u, 0u);
    runtime.WriteWord(window + 72u, 0u);
    runtime.WriteWord(window + 76u, 0u);
    runtime.Free(runtime.ReadWord(window + 48u));
    runtime.WriteWord(window + 48u, 0u);
    runtime.WriteWord(window + 52u, 0u);
    runtime.WriteWord(window + 56u, 0u);
    return runtime.DestructBase(window);
}

}  // namespace pal4
