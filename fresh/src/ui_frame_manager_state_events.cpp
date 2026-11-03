#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

std::uint8_t UiFrameManagerSingleton::ProcessStateEvent(
    const std::uint32_t manager, const std::uint32_t type,
    IUiFrameManagerStateEventRuntime& runtime) {
    const auto game = runtime.GameInstance();
    const auto controller = runtime.ReadWord(game + 0x18cu);
    if (controller == 0u) return static_cast<std::uint8_t>(game);

    const auto current_game = runtime.GameInstance();
    const auto current_controller = runtime.ReadWord(current_game + 0x18cu);
    const auto state = runtime.QueryGameState(current_controller);
    if (state != 1u) return static_cast<std::uint8_t>(state);
    return runtime.ProcessEvent(manager + 0x104u, 0x8a80a4u,
                                type & 0xffu, 0u);
}

}  // namespace pal4
