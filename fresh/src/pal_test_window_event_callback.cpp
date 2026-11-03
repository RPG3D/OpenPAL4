#include "pal4/pal_test_window_event_callback.h"

namespace pal4 {

std::uint32_t PalTestWindowEventCallback::Construct(
    std::uint32_t storage, std::uint32_t handler,
    std::uint32_t context, std::uint32_t owner,
    IPalTestWindowEventCallbackRuntime& runtime) {
    runtime.WriteWord(storage + 8u, handler);
    runtime.WriteWord(storage + 0xcu, context);
    runtime.WriteWord(storage + 0x10u, owner);
    runtime.WriteWord(storage, kVtable);
    return storage;
}

}  // namespace pal4
