#include "pal4/role_selector_window.h"

#include <array>
#include <cstdint>
#include <string>

namespace pal4 {

namespace {

std::uint32_t Subscribe(const std::uint32_t object,
                        const std::uint32_t window,
                        const RoleSelectorEvent event,
                        const std::uint32_t function,
                        IRoleSelectorWindowRuntime& runtime) {
    const auto callback = runtime.Allocate(24u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, function);
        runtime.WriteWord(callback + 12u, 0u);
        runtime.WriteWord(callback + 16u, object);
        runtime.WriteWord(callback, RoleSelectorWindow::kCallbackVtable);
    }
    try {
        runtime.Subscribe(window, event, callback);
    } catch (...) {
        if (callback != 0u)
            runtime.ReleaseSubscriptionCallback(callback);
        throw;
    }
    if (callback != 0u)
        runtime.ReleaseSubscriptionCallback(callback);
    return callback;
}

}  // namespace

std::uint32_t RoleSelectorWindow::Construct(
    const std::uint32_t object, const std::uint32_t,
    IRoleSelectorWindowRuntime& runtime) {
    unsigned stage = 0u;
    try {
        runtime.LoadLayout(object, "RoleSelectorWindow");
        stage = 1u;
        runtime.ConstructNavigation(object + 40u);
        stage = 2u;
        runtime.WriteWord(object + 64u, 0u);
        runtime.WriteWord(object + 68u, 0u);
        for (std::uint32_t index = 0u; index < 4u; ++index)
            runtime.ConstructVector2(object + 280u + index * 8u);
        for (std::uint32_t index = 0u; index < 4u; ++index)
            runtime.ConstructVector2(object + 312u + index * 8u);
        runtime.WriteWord(object, kVtable);
        runtime.WriteWord(object + 40u, kNavigationVtable);
        runtime.InitializeCloseNameBuffer();
        stage = 3u;

        const auto prefix = runtime.ReadLayoutPrefix(object);
        const auto close_button = runtime.FindWindow(
            object, prefix + "BtnClose");
        runtime.WriteWord(object + 348u, close_button);
        (void)Subscribe(object, close_button, RoleSelectorEvent::Clicked,
                        0x4b6890u, runtime);
        runtime.WriteWord(object + 344u, runtime.FindWindow(
            object, prefix + "PanelClose"));

        for (std::uint32_t index = 0u; index < 4u; ++index) {
            const auto child = object + 72u + index * 52u;
            const auto arguments = runtime.CopyPrefixArguments(object + 20u);
            (void)RoleStaticElements::Create(
                child, arguments[0], arguments[1], arguments[2],
                arguments[3], static_cast<std::int32_t>(index), runtime);

            const auto panel_position = runtime.GetPosition(
                runtime.ReadWord(child));
            runtime.WriteWord(object + 280u + index * 8u,
                              panel_position[0]);
            runtime.WriteWord(object + 284u + index * 8u,
                              panel_position[1]);
            const auto button = runtime.ReadWord(child + 28u);
            const auto button_position = runtime.GetPosition(button);
            runtime.WriteWord(object + 312u + index * 8u,
                              button_position[0]);
            runtime.WriteWord(object + 316u + index * 8u,
                              button_position[1]);
            (void)Subscribe(object, button, RoleSelectorEvent::Clicked,
                            0x4b6840u, runtime);
            runtime.SetupMouseHandlers(object + 40u, button);
        }
        runtime.LayoutButtons(object);
        const auto root = runtime.ReadWord(object + 36u);
        runtime.SetAlwaysOnTop(root, true);
        (void)Subscribe(object, root, RoleSelectorEvent::KeyDown,
                        0x4b6e10u, runtime);
        runtime.CleanupCloseNameBuffer();
        stage = 2u;
        return object;
    } catch (...) {
        if (stage >= 3u) runtime.CleanupCloseNameBuffer();
        if (stage >= 2u) runtime.DestructNavigation(object + 40u);
        if (stage >= 1u) runtime.DestructBase(object);
        throw;
    }
}

}  // namespace pal4
