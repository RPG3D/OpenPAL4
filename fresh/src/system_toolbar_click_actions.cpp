#include "pal4/system_toolbar_click_actions.h"

#include "pal4/system_toolbar_operations.h"

namespace pal4 {
namespace {

std::uint32_t FindToolbar(ISystemToolbarClickRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.FindFrame(manager,
                             runtime.ReadWord(SystemToolbarClickActions::kNameSource));
}

}  // namespace

std::uint8_t SystemToolbarClickActions::HandleClick1(
    ISystemToolbarClickRuntime& runtime) {
    const auto frame = FindToolbar(runtime);
    if (frame != 0u) {
        runtime.ClearAllSelection(frame);
    }
    runtime.DispatchEvent(runtime.FrameManager() + 260u,
                          kMainEvent, 1u, 0u);
    return 1u;
}

std::uint8_t SystemToolbarClickActions::HandleClick1(
    ISystemToolbarClickRuntime& runtime,
    ISystemToolbarOperationsRuntime& operations) {
    const auto frame = FindToolbar(runtime);
    if (frame != 0u) {
        SystemToolbarOperations::ClearAllSelection(frame, operations);
    }
    runtime.DispatchEvent(runtime.FrameManager() + 260u,
                          kMainEvent, 1u, 0u);
    return 1u;
}

bool SystemToolbarClickActions::HandleClick2(
    ISystemToolbarClickRuntime& runtime) {
    const auto frame = FindToolbar(runtime);
    if (frame != 0u) {
        runtime.Close(frame);
    }
    // The original shared constant-return body reports a handled UI event.
    return true;
}

bool SystemToolbarClickActions::HandleClick2(
    ISystemToolbarClickRuntime& runtime,
    ISystemToolbarOperationsRuntime& operations) {
    const auto frame = FindToolbar(runtime);
    if (frame != 0u) {
        (void)SystemToolbarOperations::Close(frame, operations);
    }
    return true;
}

std::uint8_t SystemToolbarClickActions::HandleClick3(
    const std::uint32_t event, ISystemToolbarClickRuntime& runtime) {
    const auto frame = FindToolbar(runtime);
    if (frame == 0u) {
        return 1u;
    }
    const auto choice = runtime.ReadWord(event);
    constexpr std::uint32_t offsets[]{0u, 72u, 64u, 76u,
                                      68u, 88u, 80u, 84u};
    if (choice >= 1u && choice <= 7u) {
        runtime.HandleMouseEvent(runtime.ReadWord(frame + offsets[choice]));
    }
    return 1u;
}

}  // namespace pal4
