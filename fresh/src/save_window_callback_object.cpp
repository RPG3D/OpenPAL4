#include "pal4/save_window_callback_object.h"

namespace pal4 {

std::uint32_t SaveWindowCallbackObject::Clone(
    std::uint32_t source, ISaveWindowCallbackObjectRuntime& runtime) {
    const auto storage = runtime.Allocate(0x18u);
    if (storage == 0u) return 0u;
    runtime.WriteWord(storage, kBaseVtable);
    runtime.WriteWord(storage + 8u, runtime.ReadWord(source + 8u));
    runtime.WriteWord(storage + 0xcu, runtime.ReadWord(source + 0xcu));
    runtime.WriteWord(storage + 0x10u, runtime.ReadWord(source + 0x10u));
    runtime.WriteWord(storage, kVtable);
    return storage;
}

std::uint32_t SaveWindowCallbackObject::Destroy(
    std::uint32_t object, std::uint8_t flags,
    ISaveWindowCallbackObjectRuntime& runtime) {
    runtime.DestroyBase(object);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

}  // namespace pal4
