#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct PackageDecoderPoolSlot final {
    std::uint32_t address = 0;
    std::uint32_t capacity = 0;
    std::uint32_t requested = 0;
    std::uint32_t high_water = 0;
    std::uint32_t use_count = 0;
    std::uint8_t initialized = 0;
    std::uint8_t occupied = 0;
    std::uint8_t touched = 0;
    std::uint8_t reserved = 0;
};
static_assert(sizeof(PackageDecoderPoolSlot) == 24u);
static_assert(offsetof(PackageDecoderPoolSlot, initialized) == 20u);

struct PackageDecoderAllocatorState final {
    std::uint32_t mode = 0;
    std::uint8_t flags = 0;
    std::uint8_t reserved[3]{};
    std::uint32_t heap = 0;
    std::uint32_t heap_bytes = 0;
    std::uint32_t reserved_region = 0;
    std::array<PackageDecoderPoolSlot, 40> slots{};
};
static_assert(sizeof(PackageDecoderAllocatorState) == 980u);
static_assert(offsetof(PackageDecoderAllocatorState, slots) == 20u);

class IPackageDecoderAllocatorRuntime {
public:
    virtual ~IPackageDecoderAllocatorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Malloc(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t HeapCreate(
        std::uint32_t options, std::uint32_t initial_bytes,
        std::uint32_t maximum_bytes) = 0;
    [[nodiscard]] virtual std::uint32_t HeapAlloc(
        std::uint32_t heap, std::uint32_t flags, std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t HeapFree(
        std::uint32_t heap, std::uint32_t flags, std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t VirtualReserve(
        std::uint32_t address, std::uint32_t bytes,
        std::uint32_t allocation_type, std::uint32_t protection) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t HeapDestroy(std::uint32_t heap) = 0;
    [[nodiscard]] virtual std::uint32_t VirtualRelease(
        std::uint32_t address, std::uint32_t bytes,
        std::uint32_t free_type) = 0;
    virtual void ReportHeapCreateFailure() = 0;
    virtual void ReportHeapDestroyFailure() = 0;
    virtual void ReportAllocationFailure(std::uint32_t bytes) = 0;
    virtual void ReportFreeFailure() = 0;
};

class PackageDecoderAllocator final {
public:
    static constexpr std::array<std::uint32_t, 5> kPoolSizes{
        0x10000u, 0x40000u, 0x100000u, 0x400000u, 0x800000u};
    static constexpr std::array<std::uint32_t, 5> kInitialCounts{
        4u, 2u, 1u, 1u, 1u};

    // IDA 0x794880.
    [[nodiscard]] static PackageDecoderAllocatorState& Initialize(
        PackageDecoderAllocatorState& state, std::uint32_t mode,
        std::uint8_t flags, IPackageDecoderAllocatorRuntime& runtime);
    // IDA 0x793C00 calls the shared cleanup body at 0x7949B0.
    static void Cleanup(PackageDecoderAllocatorState& state,
                        IPackageDecoderAllocatorRuntime& runtime);
    // IDA 0x794A40 and 0x794BD0.
    [[nodiscard]] static std::uint32_t Allocate(
        PackageDecoderAllocatorState& state, std::uint32_t bytes,
        IPackageDecoderAllocatorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Release(
        PackageDecoderAllocatorState& state, std::uint32_t address,
        std::uint32_t bytes, IPackageDecoderAllocatorRuntime& runtime);
};

}  // namespace pal4
