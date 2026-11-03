#include "pal4/heap_reallocator.h"

namespace pal4 {

std::uint32_t HeapReallocator::ReallocateWrapper(
    const std::uint32_t payload, const std::uint32_t new_size,
    const std::uint32_t hint,
    IHeapReallocRuntime& runtime) {
    (void)hint;
    return Reallocate(payload, new_size, runtime);
}
namespace {

constexpr std::uint32_t kHeap = HeapAllocator::kHeapAddress;
constexpr std::uint32_t kTop = kHeap + 0x2cu;
constexpr std::uint32_t kAllocatedBytes = kHeap + 0x394u;

void Account(const std::uint32_t payload, IHeapReallocRuntime& runtime) {
    runtime.Write(kAllocatedBytes,
                  runtime.Read(kAllocatedBytes) +
                      HeapAllocator::PayloadSize(payload, runtime));
}

void MarkFollowingChunk(const std::uint32_t address,
                        IHeapReallocRuntime& runtime) {
    runtime.Write(address, runtime.Read(address) | 1u);
}

std::uint32_t ResizeInPlace(const std::uint32_t payload,
                            const std::uint32_t total,
                            const std::uint32_t needed,
                            IHeapReallocRuntime& runtime) {
    const std::uint32_t original_header = runtime.Read(payload - 4u);
    const std::uint32_t remainder = total - needed;
    if (remainder < 16u) {
        runtime.Write(payload - 4u, total | (original_header & 1u));
        MarkFollowingChunk(payload + total - 4u, runtime);
    } else {
        const std::uint32_t remainder_payload = payload + needed;
        runtime.Write(payload - 4u, needed | (original_header & 1u));
        runtime.Write(remainder_payload - 4u, remainder | 1u);
        MarkFollowingChunk(remainder_payload + remainder - 4u, runtime);
        runtime.Free(remainder_payload);
    }
    Account(payload, runtime);
    return payload;
}

}  // namespace

std::uint32_t HeapReallocator::Reallocate(
    const std::uint32_t payload, const std::uint32_t new_size,
    IHeapReallocRuntime& runtime) {
    if (payload == 0u) {
        return runtime.Allocate(new_size);
    }
    if (new_size >= 0xffffffe0u) {
        return 0u;
    }
    const std::uint32_t needed = new_size + 11u < 16u
                                     ? 16u
                                     : (new_size + 11u) & ~7u;
    const std::uint32_t old_header = runtime.Read(payload - 4u);
    const std::uint32_t old_size = old_header & ~3u;

    if ((old_header & 2u) != 0u) {
        if (old_size >= needed + 4u) {
            return payload;
        }
        const std::uint32_t replacement = runtime.Allocate(needed - 7u);
        if (replacement != 0u) {
            runtime.Copy(replacement, payload, old_size - 8u);
            runtime.Free(payload);
        }
        return replacement;
    }

    if (old_size >= needed) {
        return ResizeInPlace(payload, old_size, needed, runtime);
    }

    const std::uint32_t adjacent = payload + old_size - 8u;
    const std::uint32_t adjacent_header = runtime.Read(adjacent + 4u);
    if (adjacent == runtime.Read(kTop)) {
        const std::uint32_t total = old_size + (adjacent_header & ~3u);
        if (total >= needed + 16u) {
            runtime.Write(payload - 4u, needed | (old_header & 1u));
            const std::uint32_t next_top = payload + needed - 8u;
            runtime.Write(kTop, next_top);
            runtime.Write(next_top + 4u, (total - needed) | 1u);
            Account(payload, runtime);
            return payload;
        }
    } else {
        const std::uint32_t after_adjacent =
            adjacent + (adjacent_header & ~1u) + 4u;
        if ((runtime.Read(after_adjacent) & 1u) == 0u) {
            const std::uint32_t total = old_size + (adjacent_header & ~3u);
            if (total >= needed) {
                const std::uint32_t forward = runtime.Read(adjacent + 8u);
                const std::uint32_t backward = runtime.Read(adjacent + 12u);
                runtime.Write(forward + 12u, backward);
                runtime.Write(backward + 8u, forward);
                return ResizeInPlace(payload, total, needed, runtime);
            }
        }
    }

    const std::uint32_t replacement = runtime.Allocate(needed - 7u);
    if (replacement == 0u) {
        return 0u;
    }
    if (replacement - 8u == adjacent) {
        const std::uint32_t total =
            old_size + (runtime.Read(replacement - 4u) & ~3u);
        return ResizeInPlace(payload, total, needed, runtime);
    }
    runtime.Copy(replacement, payload, old_size - 4u);
    runtime.Free(payload);
    // The machine-code return tail adds the new block's payload size again.
    Account(replacement, runtime);
    return replacement;
}

}  // namespace pal4
