#include "pal4/world_map_callback_bridge.h"

namespace pal4 {

std::uint32_t WorldMapCallbackBridge::Clone(
    const std::uint32_t source, IWorldMapCallbackRuntime& runtime) {
    const auto destination = runtime.Allocate(0x18u);
    if (destination == 0u) return 0u;
    runtime.WriteWord(destination, kBaseVtable);
    runtime.WriteWord(destination + 8u, runtime.ReadWord(source + 8u));
    runtime.WriteWord(destination + 0xcu, runtime.ReadWord(source + 0xcu));
    runtime.WriteWord(destination + 0x10u, runtime.ReadWord(source + 0x10u));
    runtime.WriteWord(destination, kWorldMapVtable);
    return destination;
}

std::uint32_t WorldMapCallbackBridge::Invoke(
    const std::uint32_t callback, IWorldMapCallbackRuntime& runtime) {
    const auto adjustment = runtime.ReadWord(callback + 0x10u);
    const auto receiver = runtime.ReadWord(callback + 0xcu);
    return runtime.InvokeThiscall(runtime.ReadWord(callback + 8u),
                                  receiver + adjustment);
}

std::uint32_t UIFrameManagerAccessor::Get(
    IUIFrameManagerAccessorRuntime& runtime) {
    return runtime.GetInstance();
}

}  // namespace pal4
