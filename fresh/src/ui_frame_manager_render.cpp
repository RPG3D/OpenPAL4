#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

std::uint32_t UiFrameManagerSingleton::Render(
    const std::uint32_t manager, const std::uint32_t context_record,
    IUiFrameManagerRenderRuntime& runtime) {
    const auto system = runtime.ReadWord(manager + 0x198u);
    if (system != 0u) runtime.RenderGui(system);
    return runtime.RenderAnimations(manager + 0x1dcu,
                                    runtime.ReadWord(context_record));
}

void UiFrameManagerSingleton::PreRender(
    const std::uint32_t manager, const std::uint32_t context_record,
    IUiFrameManagerRenderRuntime& runtime) {
    const auto collection = runtime.ReadWord(manager + 0x19cu);
    if (collection == 0u) return;
    runtime.GetGameObjectCount(collection, runtime.ReadWord(context_record));
    runtime.DrawEffects(manager, runtime.ReadWord(context_record));
    const auto context = runtime.ReadWord(context_record);
    const auto current_collection = runtime.ReadWord(manager + 0x19cu);
    runtime.GetGameObject(current_collection, context);
}

}  // namespace pal4
