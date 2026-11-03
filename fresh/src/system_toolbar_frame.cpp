#include "pal4/system_toolbar_frame.h"

#include <array>

namespace pal4 {
namespace {

struct ButtonSpec {
    std::string_view name;
    std::uint32_t offset;
    bool global_lookup;
};

constexpr std::array<ButtonSpec, 7> kButtons{{
    {"sysToolBar/Property", 64u, true},
    {"sysToolBar/Magic", 68u, true},
    {"sysToolBar/Role", 72u, false},
    {"sysToolBar/Equipment", 76u, false},
    {"sysToolBar/Task", 80u, false},
    {"sysToolBar/System", 84u, false},
    {"sysToolBar/Forging", 88u, false},
}};

void BindClick(const std::uint32_t frame, const std::uint32_t window,
               ISystemToolbarFrameRuntime& runtime) {
    const auto callback = runtime.Allocate(24u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, SystemToolbarFrame::kOnButtonClick);
        runtime.WriteWord(callback + 12u, 0u);
        runtime.WriteWord(callback + 16u, frame);
        runtime.WriteWord(callback, SystemToolbarFrame::kCallbackVtable);
    }
    const auto connection = runtime.Subscribe(
        window, runtime.ReadWord(SystemToolbarFrame::kClickedEvent), callback);
    if (connection != 0u) {
        runtime.ReleaseConnection(connection);
    }
}

}  // namespace

std::uint32_t SystemToolbarFrame::Construct(
    const std::uint32_t frame, const std::uint32_t name,
    ISystemToolbarFrameRuntime& runtime) {
    runtime.LoadLayout(frame, name);
    try {
        runtime.ConstructMouseHelper(frame + 40u);
        try {
            runtime.WriteWord(frame, kFrameVtable);
            runtime.WriteWord(frame + 40u, kMouseHelperVtable);
            for (const auto& button : kButtons) {
                const auto window = button.global_lookup ?
                    runtime.FindGlobalWindow(button.name) :
                    runtime.FindChildWindow(frame, button.name);
                runtime.WriteWord(frame + button.offset, window);
                BindClick(frame, window, runtime);
            }
            runtime.SetZOrderingEnabled(runtime.ReadWord(frame + 36u), false);
            constexpr std::array<std::uint32_t, 7> mouse_order{
                72u, 64u, 76u, 68u, 88u, 80u, 84u};
            for (const auto offset : mouse_order) {
                runtime.SetupMouseHandlers(frame + 40u,
                                           runtime.ReadWord(frame + offset));
            }
        } catch (...) {
            runtime.DestroyMouseHelper(frame + 40u);
            throw;
        }
    } catch (...) {
        runtime.DestroyBase(frame);
        throw;
    }
    return frame;
}

}  // namespace pal4
