#include "pal4/global_object_registry.h"

namespace pal4 {

int GlobalObjectRegistry::InitializeAndRegister(
    IGlobalObjectRegistryRuntime& runtime) {
    (void)Initialize(runtime);
    return runtime.RegisterAtExit(0x5fbfd0u);
}

std::uint32_t GlobalObjectRegistry::Initialize(
    IGlobalObjectRegistryRuntime& runtime) {
    runtime.WriteByte(kStackByte, runtime.InitialStackByte());
    const auto sentinel = runtime.Allocate(12u);
    runtime.WriteWord(sentinel, sentinel);
    runtime.WriteWord(sentinel + 4u, sentinel);
    runtime.WriteWord(kSentinel, sentinel);
    runtime.WriteWord(kCount, 0u);
    return sentinel;
}

void GlobalObjectRegistry::Cleanup(IGlobalObjectRegistryRuntime& runtime) {
    const auto guard = runtime.ReadByte(kGuard);
    if ((guard & 1u) != 0u) return;
    const auto sentinel = runtime.ReadWord(kSentinel);
    runtime.WriteByte(kGuard, static_cast<std::uint8_t>(guard | 1u));
    auto node = runtime.ReadWord(sentinel);
    while (node != sentinel) {
        const auto next = runtime.ReadWord(node);
        const auto previous = runtime.ReadWord(node + 4u);
        runtime.WriteWord(previous, next);
        runtime.WriteWord(next + 4u, previous);
        runtime.Free(node);
        const auto count = runtime.ReadWord(kCount);
        runtime.WriteWord(kCount, count - 1u);
        node = next;
    }
    runtime.Free(runtime.ReadWord(kSentinel));
    runtime.WriteWord(kSentinel, 0u);
    runtime.WriteWord(kCount, 0u);
}

}  // namespace pal4
