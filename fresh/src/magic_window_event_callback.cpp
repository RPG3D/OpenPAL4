#include "pal4/magic_window_event_callback.h"

namespace pal4 {

std::uint32_t MagicWindowEventCallback::Clone(
    std::uint32_t source, IMagicWindowEventCallbackRuntime& runtime) {
    const auto target = runtime.Allocate(0x18u);
    if (target == 0u) return 0u;
    runtime.WriteWord(target, kProvisionalVtable);
    for (std::uint32_t offset = 8u; offset <= 0x10u; offset += 4u)
        runtime.WriteWord(target + offset, runtime.ReadWord(source + offset));
    runtime.WriteWord(target, kCloneVtable);
    return target;
}

std::uint32_t MagicWindowEventCallback::ConstructPanel(
    std::uint32_t storage, std::uint32_t handler,
    std::uint32_t flags, std::uint32_t owner,
    IMagicWindowEventCallbackRuntime& runtime) {
    runtime.WriteWord(storage + 8u, handler);
    runtime.WriteWord(storage + 0xcu, flags);
    runtime.WriteWord(storage + 0x10u, owner);
    runtime.WriteWord(storage, kPanelVtable);
    return storage;
}

}  // namespace pal4
