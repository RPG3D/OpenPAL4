#include "pal4/heap_allocator.h"

#include "pal4/heap_size_class.h"

namespace pal4 {

std::uint32_t HeapAllocator::MemAlloc(const std::uint32_t requested_size,
                                      IHeapRuntime& runtime) {
    return Allocate(requested_size, runtime);
}

std::uint32_t HeapAllocator::HMalloc(const std::uint32_t requested_size,
                                     const std::uint32_t hint,
                                     IHeapRuntime& runtime) {
    (void)hint;
    return Allocate(requested_size, runtime);
}
namespace {

constexpr std::uint32_t kTop = HeapAllocator::kHeapAddress + 0x2cu;
constexpr std::uint32_t kLastRemainder = HeapAllocator::kHeapAddress + 0x30u;
constexpr std::uint32_t kUnsorted = HeapAllocator::kHeapAddress + 0x34u;
constexpr std::uint32_t kUnsortedHead = HeapAllocator::kHeapAddress + 0x40u;
constexpr std::uint32_t kBinMap = HeapAllocator::kHeapAddress + 0x334u;
constexpr std::uint32_t kAllocatedBytes = HeapAllocator::kHeapAddress + 0x394u;

std::uint32_t ChunkSize(const std::uint32_t chunk, const IHeapRuntime& runtime) {
    return runtime.Read(chunk + 4u) & ~3u;
}

void MarkFollowingChunk(const std::uint32_t chunk,
                        const std::uint32_t size, IHeapRuntime& runtime) {
    const std::uint32_t address = chunk + size + 4u;
    runtime.Write(address, runtime.Read(address) | 1u);
}

}  // namespace

std::uint32_t HeapAllocator::Bin(const std::uint32_t index) {
    return kTop + index * 8u;
}

std::uint32_t HeapAllocator::PayloadSize(
    const std::uint32_t payload, const IHeapRuntime& runtime) {
    if (payload == 0u) {
        return 0u;
    }
    const std::uint32_t header = runtime.Read(payload - 4u);
    if ((header & 2u) != 0u) {
        return (header & ~3u) - 8u;
    }
    const std::uint32_t following = payload - 4u + (header & ~1u);
    if ((runtime.Read(following) & 1u) == 0u) {
        return 0u;
    }
    return (header & ~3u) - 4u;
}

void HeapAllocator::Account(const std::uint32_t payload,
                            IHeapRuntime& runtime) {
    runtime.Write(kAllocatedBytes,
                  runtime.Read(kAllocatedBytes) + PayloadSize(payload, runtime));
}

void HeapAllocator::Unlink(const std::uint32_t chunk,
                           IHeapRuntime& runtime) {
    const std::uint32_t forward = runtime.Read(chunk + 8u);
    const std::uint32_t backward = runtime.Read(chunk + 12u);
    runtime.Write(forward + 12u, backward);
    runtime.Write(backward + 8u, forward);
}

std::uint32_t HeapAllocator::Split(const std::uint32_t chunk,
                                   const std::uint32_t chunk_size,
                                   const std::uint32_t needed,
                                   IHeapRuntime& runtime) {
    const std::uint32_t remainder = chunk_size - needed;
    if (remainder < 0x10u) {
        MarkFollowingChunk(chunk, chunk_size, runtime);
        return chunk + 8u;
    }
    const std::uint32_t next = chunk + needed;
    runtime.Write(kHeapAddress + 0x3cu, next);
    runtime.Write(kUnsortedHead, next);
    runtime.Write(next + 8u, kUnsorted);
    runtime.Write(next + 12u, kUnsorted);
    runtime.Write(chunk + 4u, needed | 1u);
    runtime.Write(next + 4u, remainder | 1u);
    runtime.Write(next + remainder, remainder);
    return chunk + 8u;
}

std::uint32_t HeapAllocator::Allocate(const std::uint32_t requested_size,
                                      IHeapRuntime& runtime) {
    if (requested_size >= 0xffffffe0u) {
        return 0u;
    }
    const std::uint32_t needed = requested_size + 11u < 0x10u
                                     ? 0x10u
                                     : (requested_size + 11u) & ~7u;

    auto allocate_from_top = [&]() -> std::uint32_t {
        if (runtime.Read(kHeapAddress) == 0u) {
            runtime.ManageBlocks(kHeapAddress);
        }
        const std::uint32_t top = runtime.Read(kTop);
        const std::uint32_t size = ChunkSize(top, runtime);
        if (size < needed + 0x10u) {
            return runtime.Extend(needed, kHeapAddress);
        }
        const std::uint32_t next = top + needed;
        runtime.Write(kTop, next);
        runtime.Write(top + 4u, needed | 1u);
        runtime.Write(next + 4u, (size - needed) | 1u);
        const std::uint32_t payload = top + 8u;
        Account(payload, runtime);
        return payload;
    };

    const std::uint32_t flags = runtime.Read(kHeapAddress);
    if ((flags & 1u) == 0u) {
        return allocate_from_top();
    }
    if (needed <= flags) {
        const std::uint32_t slot = kHeapAddress - 4u + (needed >> 3) * 4u;
        const std::uint32_t chunk = runtime.Read(slot);
        if (chunk != 0u) {
            runtime.Write(slot, runtime.Read(chunk + 8u));
            const std::uint32_t payload = chunk + 8u;
            Account(payload, runtime);
            return payload;
        }
    }

    const std::uint32_t index = needed < 0x100u
                                    ? needed >> 3
                                    : HeapSizeClass::Index(needed);
    if (needed >= 0x100u && (flags & 2u) != 0u) {
        runtime.ManageBlocks(kHeapAddress);
    }

    if (needed < 0x100u) {
        const std::uint32_t sentinel = Bin(index);
        const std::uint32_t chunk = runtime.Read(sentinel + 12u);
        if (chunk != sentinel) {
            MarkFollowingChunk(chunk, needed, runtime);
            Unlink(chunk, runtime);
            const std::uint32_t payload = chunk + 8u;
            Account(payload, runtime);
            return payload;
        }
    }

    for (std::uint32_t chunk = runtime.Read(kUnsortedHead);
         chunk != kUnsorted; chunk = runtime.Read(kUnsortedHead)) {
        const std::uint32_t size = ChunkSize(chunk, runtime);
        const std::uint32_t next = runtime.Read(chunk + 12u);
        if (needed < 0x100u && next == kUnsorted &&
            chunk == runtime.Read(kLastRemainder) &&
            size > needed + 0x10u) {
            const std::uint32_t payload = Split(chunk, size, needed, runtime);
            runtime.Write(kLastRemainder, chunk + needed);
            Account(payload, runtime);
            return payload;
        }

        runtime.Write(kUnsortedHead, next);
        runtime.Write(next + 8u, kUnsorted);
        if (size == needed) {
            MarkFollowingChunk(chunk, size, runtime);
            const std::uint32_t payload = chunk + 8u;
            Account(payload, runtime);
            return payload;
        }

        const std::uint32_t bin_index = size < 0x100u
                                            ? size >> 3
                                            : HeapSizeClass::Index(size);
        const std::uint32_t sentinel = Bin(bin_index);
        std::uint32_t forward = runtime.Read(sentinel + 8u);
        std::uint32_t backward = sentinel;
        if (size >= 0x100u && forward != sentinel) {
            if (size >= runtime.Read(runtime.Read(sentinel + 12u) + 4u)) {
                while ((size | 1u) < runtime.Read(forward + 4u)) {
                    forward = runtime.Read(forward + 8u);
                }
                backward = runtime.Read(forward + 12u);
            } else {
                forward = sentinel;
                backward = runtime.Read(sentinel + 12u);
            }
        }
        const std::uint32_t word = bin_index >> 5;
        runtime.Write(kBinMap + word * 4u,
                      runtime.Read(kBinMap + word * 4u) |
                          (1u << (bin_index & 31u)));
        runtime.Write(chunk + 8u, forward);
        runtime.Write(chunk + 12u, backward);
        runtime.Write(forward + 12u, chunk);
        runtime.Write(backward + 8u, chunk);
    }

    if (needed >= 0x100u) {
        const std::uint32_t sentinel = Bin(index);
        for (std::uint32_t chunk = runtime.Read(sentinel + 12u);
             chunk != sentinel; chunk = runtime.Read(chunk + 12u)) {
            const std::uint32_t size = ChunkSize(chunk, runtime);
            if (size < needed) {
                continue;
            }
            Unlink(chunk, runtime);
            const std::uint32_t payload = Split(chunk, size, needed, runtime);
            Account(payload, runtime);
            return payload;
        }
    }

    for (std::uint32_t bin_index = index + 1u; bin_index < 96u; ++bin_index) {
        const std::uint32_t word = bin_index >> 5;
        const std::uint32_t bit = 1u << (bin_index & 31u);
        const std::uint32_t map_address = kBinMap + word * 4u;
        if ((runtime.Read(map_address) & bit) == 0u) {
            continue;
        }
        const std::uint32_t sentinel = Bin(bin_index);
        const std::uint32_t chunk = runtime.Read(sentinel + 12u);
        if (chunk == sentinel) {
            runtime.Write(map_address, runtime.Read(map_address) & ~bit);
            continue;
        }
        const std::uint32_t size = ChunkSize(chunk, runtime);
        Unlink(chunk, runtime);
        const std::uint32_t payload = Split(chunk, size, needed, runtime);
        if (size - needed >= 0x10u && needed < 0x100u) {
            runtime.Write(kLastRemainder, chunk + needed);
        }
        Account(payload, runtime);
        return payload;
    }
    return allocate_from_top();
}

}  // namespace pal4
