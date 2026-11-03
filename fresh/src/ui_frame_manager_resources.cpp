#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

bool UiFrameManagerSingleton::AddWorldClump(
    const std::uint32_t manager, const std::uint32_t clump,
    IUiFrameManagerWorldClumpRuntime& runtime) {
    const auto collection = runtime.ReadWord(manager + 0x19cu);
    return runtime.AddClump(collection + 0x18u, clump) > 0;
}

std::uint32_t UiFrameManagerSingleton::AddUiResource(
    const std::uint32_t manager, const char* name,
    IUiFrameManagerResourceRuntime& runtime) {
    return runtime.AddElement(runtime.ReadWord(manager + 0x19cu),
                              "gamedata\\ui\\", name);
}

}  // namespace pal4
