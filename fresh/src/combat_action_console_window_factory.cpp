#include "pal4/combat_action_console_window_factory.h"

namespace pal4 {

std::uint32_t CombatActionConsoleWindowFactory::Create(
    const std::uint32_t name,
    ICombatActionConsoleWindowFactoryRuntime& runtime) {
    const auto object = runtime.Allocate(kFrameSize);
    if (object == 0u) return 0u;
    try {
        return runtime.ConstructFrame(object, name);
    } catch (...) {
        (void)runtime.Free(object);
        throw;
    }
}

}  // namespace pal4
