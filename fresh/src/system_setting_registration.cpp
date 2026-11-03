#include "pal4/system_setting_registration.h"

namespace pal4 {

std::uint32_t SystemSettingRegistration::GetHandler(
    ISystemSettingRegistrationRuntime& runtime) {
    return RegisterHandler(runtime);
}

std::uint32_t SystemSettingRegistration::RegisterHandler(
    ISystemSettingRegistrationRuntime& runtime) {
    const auto name = runtime.ReadWord(kNameSource);
    const auto handler = runtime.Allocate(8u);
    if (handler != 0u) {
        runtime.WriteWord(handler + 4u, name);
        runtime.WriteWord(handler, kHandlerVtable);
    }
    try {
        runtime.InsertOrReplace(runtime.FrameManager() + 0x104u,
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
