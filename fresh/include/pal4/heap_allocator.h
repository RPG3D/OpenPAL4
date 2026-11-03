#pragma once

#include <cstdint>

namespace pal4 {

class IHeapRuntime {
public:
    virtual ~IHeapRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
    virtual void Write(std::uint32_t address, std::uint32_t value) = 0;
    virtual void ManageBlocks(std::uint32_t heap_address) = 0;
    [[nodiscard]] virtual std::uint32_t Extend(std::uint32_t block_size,
                                               std::uint32_t heap_address) = 0;
};

class HeapAllocator final {
public:
    static constexpr std::uint32_t kHeapAddress = 0x8dd4a0u;

    [[nodiscard]] static std::uint32_t Allocate(std::uint32_t requested_size,
                                                IHeapRuntime& runtime);
    [[nodiscard]] static std::uint32_t MemAlloc(std::uint32_t requested_size,
                                                IHeapRuntime& runtime);
    [[nodiscard]] static std::uint32_t HMalloc(std::uint32_t requested_size,
                                               std::uint32_t hint,
                                               IHeapRuntime& runtime);
    [[nodiscard]] static std::uint32_t PayloadSize(std::uint32_t payload,
                                                   const IHeapRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t Bin(std::uint32_t index);
    static void Account(std::uint32_t payload, IHeapRuntime& runtime);
    static void Unlink(std::uint32_t chunk, IHeapRuntime& runtime);
    [[nodiscard]] static std::uint32_t Split(std::uint32_t chunk,
                                             std::uint32_t chunk_size,
                                             std::uint32_t needed,
                                             IHeapRuntime& runtime);
};

}  // namespace pal4
