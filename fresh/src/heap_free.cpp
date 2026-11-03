#include "pal4/heap_free.h"

namespace pal4 {

std::uint32_t HeapFree::MemFree(const std::uint32_t payload,
                                IHeapFreeRuntime& runtime) {
    return Free(payload, runtime);
}
namespace {

constexpr std::uint32_t kHeap = HeapAllocator::kHeapAddress;
constexpr std::uint32_t kTop = kHeap + 0x2cu;
constexpr std::uint32_t kUnsorted = kHeap + 0x34u;
constexpr std::uint32_t kUnsortedTail = kHeap + 0x3cu;
constexpr std::uint32_t kAllocatedBytes = kHeap + 0x394u;

void Unlink(const std::uint32_t chunk, IHeapFreeRuntime& runtime) {
    const std::uint32_t forward = runtime.Read(chunk + 8u);
    const std::uint32_t backward = runtime.Read(chunk + 12u);
    runtime.Write(forward + 12u, backward);
    runtime.Write(backward + 8u, forward);
}

}  // namespace

std::uint32_t HeapFree::Free(const std::uint32_t payload,
                             IHeapFreeRuntime& runtime) {
    const std::uint32_t payload_size =
        HeapAllocator::PayloadSize(payload, runtime);
    runtime.Write(kAllocatedBytes,
                  runtime.Read(kAllocatedBytes) - payload_size);
    if (payload == 0u) {
        return 0u;
    }

    const std::uint32_t header = runtime.Read(payload - 4u);
    std::uint32_t chunk = payload - 8u;
    std::uint32_t size = header & ~3u;
    const std::uint32_t flags = runtime.Read(kHeap);
    if (size <= flags) {
        runtime.Write(kHeap, flags | 3u);
        const std::uint32_t slot = kHeap - 4u + (size >> 3) * 4u;
        runtime.Write(payload, runtime.Read(slot));
        runtime.Write(slot, chunk);
        return chunk;
    }

    if ((header & 2u) != 0u) {
        const std::uint32_t padding = runtime.Read(chunk);
        runtime.Write(kHeap + 0x350u, runtime.Read(kHeap + 0x350u) - 1u);
        runtime.Write(kHeap + 0x364u,
                      runtime.Read(kHeap + 0x364u) - (padding + size));
        return runtime.ReleaseVirtualMemory(chunk - padding, padding + size);
    }

    runtime.Write(kHeap, flags | 1u);
    const std::uint32_t following = chunk + size;
    const std::uint32_t following_size = runtime.Read(following + 4u) & ~3u;
    if ((runtime.Read(chunk + 4u) & 1u) == 0u) {
        const std::uint32_t previous_size = runtime.Read(chunk);
        size += previous_size;
        chunk -= previous_size;
        Unlink(chunk, runtime);
    }

    if (following == runtime.Read(kTop)) {
        size += following_size;
        runtime.Write(chunk + 4u, size | 1u);
        runtime.Write(kTop, chunk);
    } else {
        const std::uint32_t following_is_in_use =
            runtime.Read(following + following_size + 4u) & 1u;
        runtime.Write(following + 4u, following_size);
        if (following_is_in_use == 0u) {
            size += following_size;
            Unlink(following, runtime);
        }
        const std::uint32_t tail = runtime.Read(kUnsortedTail);
        runtime.Write(chunk + 12u, kUnsorted);
        runtime.Write(chunk + 8u, tail);
        runtime.Write(kUnsortedTail, chunk);
        runtime.Write(tail + 12u, chunk);
        runtime.Write(chunk + 4u, size | 1u);
        runtime.Write(chunk + size, size);
    }

    if (size >= 0x20000u) {
        if ((runtime.Read(kHeap) & 2u) != 0u) {
            runtime.ManageBlocks(kHeap);
        }
        const std::uint32_t trim_threshold = runtime.Read(kHeap + 0x344u);
        if ((runtime.Read(runtime.Read(kTop) + 4u) & ~3u) >=
            trim_threshold) {
            return runtime.AdjustHeapSize(runtime.Read(kHeap + 0x348u), kHeap);
        }
        return trim_threshold;
    }
    return chunk;
}

}  // namespace pal4
