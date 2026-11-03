#include "pal4/system_setting_dialog_callback.h"

namespace pal4 {

std::uint32_t SystemSettingDialogCallback::Construct(
    const std::uint32_t storage, const std::uint32_t function,
    const std::uint32_t context, const std::uint32_t owner,
    ISystemSettingDialogCallbackRuntime& runtime) {
    runtime.WriteWord(storage + 8u, function);
    runtime.WriteWord(storage + 12u, context);
    runtime.WriteWord(storage + 16u, owner);
    runtime.WriteWord(storage, kVtable);
    return storage;
}

std::uint32_t SystemSettingDialogCallback::Clone(
    const std::uint32_t source, ISystemSettingDialogCallbackRuntime& runtime) {
    const auto storage = runtime.Allocate(24u);
    if (storage == 0u) {
        return 0u;
    }
    runtime.WriteWord(storage, kBaseVtable);
    runtime.WriteWord(storage + 8u, runtime.ReadWord(source + 8u));
    runtime.WriteWord(storage + 12u, runtime.ReadWord(source + 12u));
    runtime.WriteWord(storage + 16u, runtime.ReadWord(source + 16u));
    runtime.WriteWord(storage, kVtable);
    return storage;
}

}  // namespace pal4
