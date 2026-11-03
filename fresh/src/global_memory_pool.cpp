#include "pal4/global_memory_pool.h"

#include <array>

namespace pal4 {

std::uint32_t MemoryPoolInitializer::Initialize(
    const std::uint32_t pool, const std::uint32_t,
    const std::uint32_t requested_size, const std::uint32_t alignment,
    IGlobalMemoryPoolRuntime& runtime) {
    const std::uint32_t size = requested_size < 0x1000u
                                   ? 0x1000u : requested_size;
    runtime.Write(pool, size);
    runtime.Write(pool + 4u, alignment);
    runtime.Write(pool + 20u, 0u);
    runtime.Write(pool + 24u, 0u);
    runtime.WriteByte(pool + 28u,
                      static_cast<std::uint8_t>(requested_size));
    runtime.Write(pool + 32u, 0u);
    runtime.Write(pool + 36u, 0u);
    runtime.Write(pool + 40u, 0u);
    runtime.Write(pool + 44u, 0u);
    runtime.Write(pool + 16u, size);

    std::uint32_t normalized = alignment & ~3u;
    if (normalized < 4u) {
        normalized = 4u;
    }
    std::uint32_t power_of_two = 1u;
    while (power_of_two < normalized) {
        power_of_two += power_of_two;
    }
    runtime.Write(pool + 4u, power_of_two);
    const std::uint32_t mask = power_of_two - 1u;
    runtime.Write(pool + 8u, mask);
    runtime.Write(pool + 12u, ~mask & (mask + 8u));
    return pool;
}

std::uint32_t GlobalMemoryPool::Initialize(
    IGlobalMemoryPoolRuntime& runtime) {
    return MemoryPoolInitializer::Initialize(
        kAddress, 0u, 0x2000u, 16u, runtime);
}

std::uint32_t GlobalMemoryPool::InitializeAndRegisterExit(
    IGlobalMemoryPoolRuntime& runtime) {
    (void)Initialize(runtime);
    return runtime.RegisterExitHandler(kCleanupEntry);
}

void GlobalMemoryPool::Cleanup(IGlobalMemoryPoolRuntime& runtime) {
    for (const std::uint32_t offset :
         std::array<std::uint32_t, 2>{24u, 20u}) {
        std::uint32_t node = runtime.Read(kAddress + offset);
        while (node != 0u) {
            const std::uint32_t next = runtime.Read(node);
            runtime.Free(node);
            runtime.Write(kAddress + offset, next);
            node = next;
        }
    }
    runtime.Free(runtime.Read(kAddress + 32u));
    runtime.Write(kAddress + 32u, 0u);
    runtime.Write(kAddress + 36u, 0u);
    runtime.Write(kAddress + 40u, 0u);
}

}  // namespace pal4
