#include "pal4/world_map_registration.h"

namespace pal4 {

std::uint32_t WorldMapRegistration::GetHandler(
    IWorldMapRegistrationRuntime& runtime) {
    return RegisterHandler(runtime);
}

std::uint32_t WorldMapRegistration::RegisterHandler(
    IWorldMapRegistrationRuntime& runtime) {
    const auto name = runtime.ReadWord(kNameSource);
    const auto handler = runtime.Allocate(8u);
    if (handler != 0u) {
        runtime.WriteWord(handler + 4u, name);
        runtime.WriteWord(handler, kHandlerVtable);
    }
    try {
        runtime.InsertOrReplace(runtime.FrameManager() + 260u,
                                kEventKey, handler);
    } catch (...) {
        if (handler != 0u) {
            runtime.ReleaseHandler(handler, 1u);
        }
        throw;
    }
    if (handler != 0u) {
        runtime.ReleaseHandler(handler, 1u);
    }
    return kStorage;
}

}  // namespace pal4
