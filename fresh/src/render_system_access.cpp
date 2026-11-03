#include "pal4/render_system_access.h"

namespace pal4 {

std::uint32_t RenderSystemAccess::Get(
    RenderSystemAccessState& state,
    IRenderSystemAccessRuntime& runtime) {
    if ((state.guard & 1u) != 0u) {
        return state.instance;
    }
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    state.instance = runtime.Register(
        runtime.GetSingletonManager(), "rwcWindows", 0x14u,
        kFactoryAddress, kDestructorAddress);
    return state.instance;
}

}  // namespace pal4
