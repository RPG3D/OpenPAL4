#include "pal4/game_input_dispatch.h"

namespace pal4 {

std::uint32_t GameInputDispatch::HandleKeyboard(
    IGameInputCallbackRuntime& runtime) const {
    return config_[7] != 0u ? runtime.Invoke(config_[8]) : 2u;
}

std::uint32_t GameInputDispatch::HandleMouse(
    IGameInputCallbackRuntime& runtime) const {
    return config_[10] != 0u ? runtime.Invoke(config_[11]) : 2u;
}

}  // namespace pal4
