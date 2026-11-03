#include "pal4/memory_heap_extend.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kPageCache = 0x8dd81cu;
constexpr std::uint32_t kPageAlignment = 0x8dd820u;
constexpr std::uint32_t kGranularityCache = 0x8dd498u;
constexpr std::uint32_t kGranularityAlignment = 0x8dd824u;
constexpr std::uint32_t kHead = 0x8dd840u;
constexpr std::uint32_t kCommit = 0x1000u;
constexpr std::uint32_t kReserve = 0x2000u;
constexpr std::uint32_t kDecommit = 0x4000u;
constexpr std::uint32_t kRelease = 0x8000u;

std::uint32_t RoundUp(const std::uint32_t value,
                      const std::uint32_t alignment) {
    return (value + alignment - 1u) & ~(alignment - 1u);
}

bool InitializeCaches(IMemoryHeapExtendRuntime& runtime) {
    if (runtime.Read(kPageCache) == 0u) {
        const std::uint32_t page = runtime.PageSize();
        runtime.Write(kPageCache, page);
        runtime.Write(kPageAlignment, page);
    }
    if (runtime.Read(kGranularityCache) == 0u) {
        const std::uint32_t granularity = runtime.AllocationGranularity();
        runtime.Write(kGranularityCache, granularity);
        runtime.Write(kGranularityAlignment, granularity);
    }
    if (runtime.Read(kHead) == 0u &&
        !runtime.InitializeStructure(0u, 0u)) {
        return false;
    }
    return true;
}

std::uint32_t Grow(const std::uint32_t size,
                   IMemoryHeapExtendRuntime& runtime) {
    std::uint32_t node = runtime.Read(kHead);
    const std::uint32_t current = runtime.Read(node);
    const std::uint32_t committed = runtime.Read(node + 4u);
    const std::uint32_t page = runtime.Read(kPageAlignment);
    const std::uint32_t excess = size + current - committed;
    if (static_cast<std::int32_t>(excess) <= 0) {
        runtime.Write(node, current + size);
        return current;
    }

    std::uint32_t commit_size = RoundUp(excess, page);
    const std::uint32_t reserved_end = runtime.Read(node + 8u);
    const std::uint32_t extra_reserve = committed + commit_size - reserved_end;
    if (static_cast<std::int32_t>(extra_reserve) > 0) {
        const std::uint32_t reserved_uncommitted = reserved_end - committed;
        if (static_cast<std::int32_t>(reserved_uncommitted) > 0) {
            const std::uint32_t result = runtime.VirtualAllocate(
                committed, reserved_uncommitted, kCommit, 4u);
            if (result != committed) {
                return MemoryHeapExtend::kFailure;
            }
            node = runtime.Read(kHead);
            runtime.Write(node + 4u, result + reserved_uncommitted);
        }

        bool saw_candidate = false;
        for (;;) {
            node = runtime.Read(kHead);
            std::uint32_t address = runtime.Read(node + 8u);
            std::uint32_t reserve_size = RoundUp(
                extra_reserve, runtime.Read(kGranularityAlignment));
            bool contiguous = true;
            VirtualMemoryRegion region;
            region.base = address;
            bool queried = runtime.VirtualQuery(address, region);
            while (queried) {
                if (region.state == 0x10000u &&
                    region.base % runtime.Read(kGranularityCache) == 0u &&
                    region.size >= reserve_size) {
                    saw_candidate = true;
                    break;
                }
                contiguous = false;
                reserve_size = RoundUp(size,
                                       runtime.Read(kGranularityAlignment));
                address = region.base + region.size;
                region.base = address;
                queried = runtime.VirtualQuery(address, region);
            }
            if (!queried && !saw_candidate) {
                return MemoryHeapExtend::kFailure;
            }
            const std::uint32_t target = region.base;
            const std::uint32_t reserved = runtime.VirtualAllocate(
                target, reserve_size, kReserve, 1u);
            if (reserved == 0u) {
                if (runtime.LastError() != 487u) {
                    return MemoryHeapExtend::kFailure;
                }
                continue;
            }
            if (target != 0u && reserved != target) {
                return MemoryHeapExtend::kFailure;
            }

            std::uint32_t increment = size;
            if (contiguous) {
                node = runtime.Read(kHead);
                const std::uint32_t old_current = runtime.Read(node);
                const std::uint32_t old_commit = runtime.Read(node + 4u);
                runtime.Write(node, old_commit);
                increment = size + old_current - old_commit;
                commit_size = RoundUp(increment, page);
            }
            if (!runtime.InitializeStructure(reserved, reserve_size)) {
                return MemoryHeapExtend::kFailure;
            }
            node = runtime.Read(kHead);
            if (!contiguous) {
                commit_size = RoundUp(runtime.Read(node) -
                                          runtime.Read(node + 4u) + increment,
                                      page);
            }
            const std::uint32_t commit_address = runtime.Read(node + 4u);
            const std::uint32_t committed_result = runtime.VirtualAllocate(
                commit_address, commit_size, kCommit, 4u);
            if (committed_result != commit_address) {
                return MemoryHeapExtend::kFailure;
            }
            node = runtime.Read(kHead);
            runtime.Write(node + 4u, committed_result + commit_size);
            runtime.Write(node, runtime.Read(node) + increment);
            return runtime.Read(node) - size;
        }
    }

    const std::uint32_t result = runtime.VirtualAllocate(
        committed, commit_size, kCommit, 4u);
    if (result != committed) {
        return MemoryHeapExtend::kFailure;
    }
    node = runtime.Read(kHead);
    runtime.Write(node + 4u, result + commit_size);
    runtime.Write(node, runtime.Read(node) + size);
    return runtime.Read(node) - size;
}

std::uint32_t Shrink(const std::uint32_t signed_size,
                     IMemoryHeapExtendRuntime& runtime) {
    std::uint32_t node = runtime.Read(kHead);
    std::uint32_t shrink = 0u - signed_size;
    std::uint32_t region_start =
        runtime.Read(node + 8u) - runtime.Read(node + 12u);
    while (runtime.Read(node) - shrink < region_start) {
        if (!runtime.VirtualRelease(region_start, 0u, kRelease)) {
            return MemoryHeapExtend::kFailure;
        }
        shrink += region_start - runtime.Read(node);
        if (!runtime.FreeStructure()) {
            return MemoryHeapExtend::kFailure;
        }
        node = runtime.Read(kHead);
        region_start = runtime.Read(node + 8u) - runtime.Read(node + 12u);
    }

    std::uint32_t committed = runtime.Read(node + 4u);
    const std::uint32_t current = runtime.Read(node);
    const std::uint32_t page = runtime.Read(kPageAlignment);
    const std::uint32_t excess = shrink + committed - current;
    if (static_cast<std::int32_t>(excess) >=
        static_cast<std::int32_t>(page)) {
        const std::uint32_t decommit_size = excess & ~(page - 1u);
        const std::uint32_t decommit_at = committed - decommit_size;
        if (!runtime.VirtualRelease(decommit_at, decommit_size, kDecommit)) {
            return MemoryHeapExtend::kFailure;
        }
        node = runtime.Read(kHead);
        shrink += decommit_at - runtime.Read(node);
        runtime.Write(node + 4u, decommit_at);
        runtime.Write(node, decommit_at);
        committed = decommit_at;
    }
    node = runtime.Read(kHead);
    const std::uint32_t next = runtime.Read(node) - shrink;
    runtime.Write(node, next);
    region_start = runtime.Read(node + 8u) - runtime.Read(node + 12u);
    if (region_start <= next && next <= committed) {
        return next;
    }
    runtime.Write(node, region_start);
    return MemoryHeapExtend::kFailure;
}

}  // namespace

std::uint32_t MemoryHeapExtend::Change(
    const std::uint32_t signed_size, IMemoryHeapExtendRuntime& runtime) {
    if (!InitializeCaches(runtime)) {
        return kFailure;
    }
    return static_cast<std::int32_t>(signed_size) < 0
               ? Shrink(signed_size, runtime)
               : Grow(signed_size, runtime);
}

}  // namespace pal4
