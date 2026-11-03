#include "pal4/ui_exit_game.h"

namespace pal4 {

std::uint8_t UiExitGameHandler::Handle(
    const std::uint32_t, IUiExitGameRuntime& runtime) {
    runtime.HandleExitCommand(runtime.GameInstance());
    return 1u;
}

}  // namespace pal4
