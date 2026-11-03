#include "pal4/smith_window_dialog_callback.h"

#include <cstdint>

namespace pal4 {

std::uint32_t SmithWindowDialogCallback::Initialize(
    const std::uint32_t object, const std::uint32_t handler,
    const std::uint32_t argument, const std::uint32_t owner,
    ISmithWindowDialogCallbackRuntime& runtime) {
    runtime.WriteWord(object + 8u, handler);
    runtime.WriteWord(object + 0xcu, argument);
    runtime.WriteWord(object + 0x10u, owner);
    runtime.WriteWord(object, kVtable);
    return object;
}

std::uint32_t SmithWindowDialogCallback::Clone(
    const std::uint32_t source,
    ISmithWindowDialogCallbackRuntime& runtime) {
    const auto clone = runtime.Allocate(kSize);
    if (clone == 0u) return 0u;
    try {
        runtime.WriteWord(clone, kBaseVtable);
        runtime.WriteWord(clone + 8u,
                          runtime.ReadWord(source + 8u));
        runtime.WriteWord(clone + 0xcu,
                          runtime.ReadWord(source + 0xcu));
        runtime.WriteWord(clone + 0x10u,
                          runtime.ReadWord(source + 0x10u));
        runtime.WriteWord(clone, kVtable);
    } catch (...) {
        runtime.Free(clone);
        throw;
    }
    return clone;
}

}  // namespace pal4
