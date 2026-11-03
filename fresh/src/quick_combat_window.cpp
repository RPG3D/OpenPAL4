#include "pal4/quick_combat_window.h"

namespace pal4 {

std::uint32_t QuickCombatWindow::Construct(
    const std::uint32_t window, const std::uint32_t layout,
    IQuickCombatWindowRuntime& runtime) {
    runtime.LoadLayout(window, layout);
    runtime.WriteWord(window, kVtable);
    return window;
}

std::uint32_t QuickCombatFactory::GetTypeThunk(
    IQuickCombatWindowRuntime& runtime) {
    return GetType(runtime);
}

std::uint32_t QuickCombatFactory::GetType(
    IQuickCombatWindowRuntime& runtime) {
    const auto type = runtime.ReadWord(kTypePointer);
    runtime.WriteWord(kTypeCache, type);
    return type;
}

std::uint32_t QuickCombatFactory::Create(
    const std::uint32_t layout, IQuickCombatWindowRuntime& runtime) {
    const auto window = runtime.Allocate(0x28u);
    return window ? QuickCombatWindow::Construct(window, layout, runtime) : 0u;
}

std::uint32_t QuickCombatFactory::RegisterFactoryThunk(
    IQuickCombatWindowRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t QuickCombatFactory::RegisterFactory(
    IQuickCombatWindowRuntime& runtime) {
    (void)runtime.FrameManager();
    return runtime.RegisterFactory(runtime.ReadWord(kTypeCache),
                                   kCreateCallback);
}

std::uint32_t QuickCombatFactory::RegisterShowEventThunk(
    IQuickCombatWindowRuntime& runtime) {
    return RegisterShowEvent(runtime);
}

std::uint32_t QuickCombatFactory::RegisterShowEvent(
    IQuickCombatWindowRuntime& runtime) {
    const auto handler = runtime.Allocate(8u);
    if (handler) {
        runtime.WriteWord(handler + 4u, runtime.ReadWord(kTypePointer));
        runtime.WriteWord(handler, 0x843494u);
    }
    runtime.InsertShowEvent(runtime.FrameManager() + 260u,
                            kShowEventKey, handler);
    if (handler) runtime.DeleteHandler(handler, 1u);
    return kShowEventSentinel;
}

}  // namespace pal4
