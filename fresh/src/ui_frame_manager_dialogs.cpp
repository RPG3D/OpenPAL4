#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {
namespace {

std::uint32_t GetCachedFrame(
    const std::uint32_t manager, const std::uint32_t offset,
    const std::uint32_t size, const UiCachedFrameKind kind,
    const char* name, IUiFrameManagerCachedFrameRuntime& runtime) {
    const auto field = manager + offset;
    if (runtime.ReadWord(field) == 0u) {
        const auto memory = runtime.Allocate(size);
        auto dialog = 0u;
        if (memory != 0u) {
            try {
                dialog = runtime.Construct(kind, memory, name);
            } catch (...) {
                runtime.FreeAfterConstructionFailure(memory);
                throw;
            }
        }
        runtime.WriteWord(field, dialog);
    }
    return runtime.ReadWord(field);
}

}  // namespace

std::uint32_t UiFrameManagerSingleton::CreateCommonDialog(
    const std::uint32_t manager, IUiFrameManagerDialogRuntime& runtime) {
    const auto dialog = GetCachedFrame(manager, 0x1acu, 0x60u,
                                        UiCachedFrameKind::Common,
                                        "CommonDialog", runtime);
    runtime.ReleaseCommonResources(dialog);
    return runtime.ReadWord(manager + 0x1acu);
}

std::uint32_t UiFrameManagerSingleton::CreateYesNoDialog(
    const std::uint32_t manager, IUiFrameManagerDialogRuntime& runtime) {
    return GetCachedFrame(manager, 0x1b0u, 0x50u,
                           UiCachedFrameKind::YesNo,
                           "CommonDialog2", runtime);
}

std::uint32_t UiFrameManagerSingleton::CreateSelectDialog(
    const std::uint32_t manager, IUiFrameManagerDialogRuntime& runtime) {
    const auto dialog = GetCachedFrame(manager, 0x1ccu, 0xa4u,
                                        UiCachedFrameKind::Select,
                                        nullptr, runtime);
    runtime.CleanupSelectCallback(dialog);
    return runtime.ReadWord(manager + 0x1ccu);
}

std::uint32_t UiFrameManagerSingleton::CreateScenarioDialog(
    const std::uint32_t manager,
    IUiFrameManagerCachedFrameRuntime& runtime) {
    return GetCachedFrame(manager, 0x1b4u, 0x50u,
                          UiCachedFrameKind::Scenario, nullptr, runtime);
}

std::uint32_t UiFrameManagerSingleton::CreateProgressBar(
    const std::uint32_t manager,
    IUiFrameManagerCachedFrameRuntime& runtime) {
    return GetCachedFrame(manager, 0x1b8u, 0x38u,
                          UiCachedFrameKind::ProgressBar, nullptr, runtime);
}

std::uint32_t UiFrameManagerSingleton::CreateLoadingWindow(
    const std::uint32_t manager,
    IUiFrameManagerCachedFrameRuntime& runtime) {
    return GetCachedFrame(manager, 0x1bcu, 0x3cu,
                          UiCachedFrameKind::Loading, nullptr, runtime);
}

std::uint32_t UiFrameManagerSingleton::CreateHintWindow(
    const std::uint32_t manager,
    IUiFrameManagerCachedFrameRuntime& runtime) {
    return GetCachedFrame(manager, 0x1c0u, 0x48u,
                          UiCachedFrameKind::Hint, "hintWindow", runtime);
}

std::uint32_t UiFrameManagerSingleton::CreateCombatHintWindow(
    const std::uint32_t manager,
    IUiFrameManagerCachedFrameRuntime& runtime) {
    return GetCachedFrame(manager, 0x1c4u, 0x48u,
                          UiCachedFrameKind::CombatHint,
                          "CombathintWindow", runtime);
}

std::uint32_t UiFrameManagerSingleton::CreateCombatHintWindow2(
    const std::uint32_t manager,
    IUiFrameManagerCachedFrameRuntime& runtime) {
    return GetCachedFrame(manager, 0x1c8u, 0x48u,
                          UiCachedFrameKind::CombatHint2,
                          nullptr, runtime);
}

}  // namespace pal4
