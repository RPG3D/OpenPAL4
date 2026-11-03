#include "pal4/heap_extender.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kFailure = 0xffffffffu;
constexpr std::uint32_t kAllocatedBytes = 0x8dd834u;

void UpdateMaximum(const std::uint32_t heap, const std::uint32_t current_offset,
                   const std::uint32_t maximum_offset,
                   IHeapExtensionRuntime& runtime) {
    const std::uint32_t current = runtime.Read(heap + current_offset);
    if (current > runtime.Read(heap + maximum_offset)) {
        runtime.Write(heap + maximum_offset, current);
    }
}

std::uint32_t SplitTop(const std::uint32_t size, const std::uint32_t heap,
                       IHeapExtensionRuntime& runtime) {
    const std::uint32_t top = runtime.Read(heap + 44u);
    const std::uint32_t available = runtime.Read(top + 4u) & ~3u;
    if (available < size + 16u) {
        return 0u;
    }
    runtime.Write(heap + 44u, top + size);
    runtime.Write(top + 4u, size | 1u);
    runtime.Write(top + size + 4u, (available - size) | 1u);
    runtime.Write(kAllocatedBytes,
                  runtime.Read(kAllocatedBytes) + size - 4u);
    return top + 8u;
}

}  // namespace

std::uint32_t HeapExtender::Allocate(const std::uint32_t block_size,
                                     const std::uint32_t heap,
                                     IHeapExtensionRuntime& runtime) {
    const std::uint32_t granularity = runtime.Read(heap + 860u);
    const std::uint32_t mask = granularity - 1u;
    if ((runtime.Read(heap) & 2u) != 0u) {
        runtime.ManageBlocks(heap);
        return runtime.RetryAllocate(block_size - 7u);
    }

    if (block_size >= runtime.Read(heap + 844u) &&
        runtime.Read(heap + 848u) < runtime.Read(heap + 852u)) {
        const std::uint32_t mapped_size = (mask + block_size + 11u) & ~mask;
        if (mapped_size > block_size) {
            const std::uint32_t raw = runtime.AllocateVirtualMemory(mapped_size);
            if (raw != kFailure) {
                const std::uint32_t padding = (8u - (raw & 7u)) & 7u;
                const std::uint32_t chunk = raw + padding;
                runtime.Write(chunk, padding);
                runtime.Write(chunk + 4u, (mapped_size - padding) | 2u);
                runtime.Write(heap + 848u, runtime.Read(heap + 848u) + 1u);
                UpdateMaximum(heap, 848u, 856u, runtime);
                runtime.Write(heap + 868u,
                              runtime.Read(heap + 868u) + mapped_size);
                UpdateMaximum(heap, 868u, 880u, runtime);
                const std::uint32_t total = runtime.Read(heap + 868u) +
                                            runtime.Read(heap + 872u);
                if (total > runtime.Read(heap + 884u)) {
                    runtime.Write(heap + 884u, total);
                }
                const std::uint32_t payload = chunk + 8u;
                runtime.Write(kAllocatedBytes,
                              runtime.Read(kAllocatedBytes) +
                                  (runtime.Read(chunk + 4u) & ~3u) - 8u);
                return payload;
            }
        }
    }

    const std::uint32_t old_top = runtime.Read(heap + 44u);
    const std::uint32_t old_size = runtime.Read(old_top + 4u) & ~3u;
    const std::uint32_t old_end = old_top + old_size;
    std::uint32_t request = runtime.Read(heap + 840u) + block_size + 16u;
    if ((runtime.Read(heap + 864u) & 1u) != 0u) {
        request -= old_size;
    }
    std::uint32_t span = (mask + request) & ~mask;
    std::uint32_t start = kFailure;
    std::uint32_t end = kFailure;
    if (static_cast<std::int32_t>(span) > 0) {
        start = runtime.ExtendProgramBreak(span);
    }
    if (start == kFailure) {
        if ((runtime.Read(heap + 864u) & 1u) != 0u) {
            span = (mask + span + old_size) & ~mask;
        }
        if (span < 0x100000u) {
            span = 0x100000u;
        }
        if (span <= block_size) {
            return 0u;
        }
        start = runtime.AllocateVirtualMemory(span);
        if (start == kFailure) {
            return 0u;
        }
        end = start + span;
        runtime.Write(heap + 864u, runtime.Read(heap + 864u) & ~1u);
    }

    runtime.Write(heap + 872u, runtime.Read(heap + 872u) + span);
    if (start == old_end && end == kFailure) {
        runtime.Write(old_top + 4u, (old_size + span) | 1u);
    } else {
        const std::uint32_t region_start = start;
        std::uint32_t aligned_start = start;
        std::uint32_t correction = 0u;
        std::uint32_t morecore_flags = runtime.Read(heap + 864u);
        if ((morecore_flags & 1u) != 0u && old_size != 0u && start < old_end) {
            morecore_flags &= ~1u;
            runtime.Write(heap + 864u, morecore_flags);
        }
        if ((morecore_flags & 1u) != 0u) {
            if (old_size != 0u) {
                runtime.Write(heap + 872u,
                              region_start + runtime.Read(heap + 872u) - old_end);
            }
            const std::uint32_t padding = (8u - (region_start & 7u)) & 7u;
            aligned_start += padding;
            correction = ((mask + span + region_start + old_size + padding) &
                          ~mask) - (span + region_start);
            end = runtime.ExtendProgramBreak(correction);
            if (end == kFailure) {
                correction = 0u;
                end = runtime.ExtendProgramBreak(0u);
            } else if (end < region_start) {
                correction = 0u;
                end = region_start + span;
                runtime.Write(heap + 864u, runtime.Read(heap + 864u) & ~1u);
            }
        } else if (end == kFailure) {
            end = runtime.ExtendProgramBreak(0u);
            runtime.Write(heap + 872u,
                          runtime.Read(heap + 872u) + end - region_start - span);
        }

        if (end != kFailure) {
            runtime.Write(heap + 44u, aligned_start);
            runtime.Write(aligned_start + 4u,
                          (correction + end - aligned_start) | 1u);
            runtime.Write(heap + 872u, runtime.Read(heap + 872u) + correction);
            if (old_size != 0u) {
                const std::uint32_t usable_old = (old_size - 12u) & ~7u;
                runtime.Write(old_top + 4u, usable_old | 1u);
                runtime.Write(old_top + usable_old + 4u, 5u);
                runtime.Write(old_top + usable_old + 8u, 5u);
                if (usable_old >= 16u) {
                    const std::uint32_t saved = runtime.Read(heap + 836u);
                    runtime.Write(heap + 836u, kFailure);
                    runtime.FreeBlock(old_top + 8u);
                    runtime.Write(heap + 836u, saved);
                }
            }
        }
    }

    UpdateMaximum(heap, 872u, 876u, runtime);
    const std::uint32_t total = runtime.Read(heap + 868u) +
                                runtime.Read(heap + 872u);
    if (total > runtime.Read(heap + 884u)) {
        runtime.Write(heap + 884u, total);
    }
    return SplitTop(block_size, heap, runtime);
}

}  // namespace pal4
