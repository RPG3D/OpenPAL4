#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>

namespace pal4 {

std::uint32_t UiFrameManagerSingleton::GetInstance(
    std::uint8_t& guard, std::uint32_t& cached,
    IUiFrameManagerSingletonRuntime& runtime) {
    if ((guard & 1u) != 0u) return cached;
    guard = static_cast<std::uint8_t>(guard | 1u);
    cached = runtime.Register(runtime.SingletonManager(),
                              "uiFrameManager", 0x3ffu,
                              kFactoryAddress);
    return cached;
}

std::uint32_t UiFrameManagerSingleton::CreateInstance(
    IUiFrameManagerSingletonRuntime& runtime) {
    const auto address = runtime.Allocate(0x224u);
    if (address == 0u) return 0u;
    try {
        return runtime.Construct(address);
    } catch (...) {
        runtime.Free(address);
        throw;
    }
}

}  // namespace pal4
