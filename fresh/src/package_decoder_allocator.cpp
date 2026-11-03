#include "pal4/package_decoder_allocator.h"

namespace pal4 {

PackageDecoderAllocatorState& PackageDecoderAllocator::Initialize(
    PackageDecoderAllocatorState& state, const std::uint32_t mode,
    const std::uint8_t flags, IPackageDecoderAllocatorRuntime& runtime) {
    state.mode = mode;
    state.flags = flags;
    state.heap = 0u;
    state.heap_bytes = 0u;
    state.reserved_region = 0u;
    state.slots.fill({});

    switch (mode) {
    case 2u:
        for (std::size_t group = 0; group < kPoolSizes.size(); ++group) {
            for (std::uint32_t index = 0; index < kInitialCounts[group];
                 ++index) {
                auto& slot = state.slots[group * 8u + index];
                slot.address = runtime.Malloc(kPoolSizes[group]);
                if (slot.address != 0u) {
                    slot.capacity = kPoolSizes[group];
                    slot.initialized = 1u;
                }
            }
        }
        break;
    case 3u:
        state.heap = runtime.HeapCreate(flags != 0u ? 5u : 1u,
                                        0x1000u, 0u);
        state.heap_bytes = 0x1000u;
        if (state.heap == 0u) runtime.ReportHeapCreateFailure();
        break;
    case 5u:
        state.reserved_region = runtime.VirtualReserve(
            0u, 0x20000000u, 0x2000u, 1u);
        break;
    default:
        break;
    }
    return state;
}

void PackageDecoderAllocator::Cleanup(
    PackageDecoderAllocatorState& state,
    IPackageDecoderAllocatorRuntime& runtime) {
    switch (state.mode) {
    case 2u:
        for (auto& slot : state.slots) {
            if (slot.initialized != 0u)
                runtime.Free(slot.address);
        }
        break;
    case 3u:
        if (runtime.HeapDestroy(state.heap) != 1u)
            runtime.ReportHeapDestroyFailure();
        state.heap = 0u;
        break;
    case 5u:
        (void)runtime.VirtualRelease(state.reserved_region, 0u, 0x8000u);
        break;
    default:
        break;
    }
}

std::uint32_t PackageDecoderAllocator::Allocate(
    PackageDecoderAllocatorState& state, const std::uint32_t bytes,
    IPackageDecoderAllocatorRuntime& runtime) {
    std::uint32_t address = 0u;
    switch (state.mode) {
    case 1u:
        address = runtime.Malloc(bytes);
        break;
    case 2u: {
        std::size_t group = 0;
        while (group < kPoolSizes.size() && bytes > kPoolSizes[group])
            ++group;
        if (group == kPoolSizes.size()) {
            (void)runtime.Malloc(bytes);
            break;
        }
        for (std::size_t index = 0; index < 8u; ++index) {
            auto& slot = state.slots[group * 8u + index];
            if (slot.occupied != 0u) continue;
            if (slot.initialized == 0u)
                slot.address = runtime.Malloc(kPoolSizes[group]);
            if (slot.address == 0u) continue;
            slot.initialized = 1u;
            slot.occupied = 1u;
            slot.touched = 1u;
            slot.capacity = kPoolSizes[group];
            slot.requested = bytes;
            ++slot.use_count;
            if (bytes > slot.high_water) slot.high_water = bytes;
            address = slot.address;
            break;
        }
        if (address == 0u) (void)runtime.Malloc(bytes);
        break;
    }
    case 3u:
        address = runtime.HeapAlloc(state.heap, state.flags != 0u ? 5u : 1u,
                                    bytes);
        break;
    case 5u:
        address = runtime.VirtualReserve(state.reserved_region, bytes,
                                         0x1000u, 4u);
        break;
    default:
        break;
    }
    if (address == 0u) runtime.ReportAllocationFailure(bytes);
    return address;
}

std::uint32_t PackageDecoderAllocator::Release(
    PackageDecoderAllocatorState& state, const std::uint32_t address,
    const std::uint32_t bytes, IPackageDecoderAllocatorRuntime& runtime) {
    switch (state.mode) {
    case 1u:
        runtime.Free(address);
        return 1u;
    case 2u:
        for (std::size_t group = 0; group < kPoolSizes.size(); ++group) {
            for (std::size_t index = 0; index < 8u; ++index) {
                auto& slot = state.slots[group * 8u + index];
                if (slot.address == 0u) break;
                if (slot.address == address) {
                    slot.occupied = 0u;
                    slot.requested = 0u;
                    return 1u;
                }
            }
        }
        runtime.Free(address);
        return 1u;
    case 3u: {
        const auto result = runtime.HeapFree(
            state.heap, state.flags == 1u ? 5u : 1u, address);
        if (result == 0u) runtime.ReportFreeFailure();
        return result;
    }
    case 5u: {
        const auto result = runtime.VirtualRelease(
            state.reserved_region, bytes, 0x4000u);
        if (result == 0u) runtime.ReportFreeFailure();
        return result;
    }
    default:
        runtime.ReportFreeFailure();
        return 0u;
    }
}

}  // namespace pal4
