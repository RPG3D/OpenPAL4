#include "pal4/game_manager_component_callback.h"

namespace pal4 {

std::uint32_t GameManagerComponentCallback::Construct(
    const std::uint32_t object,
    IGameManagerComponentCallbackRuntime& runtime) {
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteWord(object, 0x849398u);
    return object;
}

}  // namespace pal4
