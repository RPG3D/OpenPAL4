#pragma once

#include <cstdint>

namespace pal4 {

class IInventoryBaseStorageRuntime {
public:
    virtual ~IInventoryBaseStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
};

class InventoryBaseStorage final {
public:
    static constexpr std::uint32_t kVtable = 0x84346cu;
    static constexpr std::uint32_t kBaseVtable = 0x84347cu;

    // IDA 0x4c6e70: the seed bytes represent unspecified native stack data.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint8_t element_seed,
        std::uint8_t tail_seed, IInventoryBaseStorageRuntime& runtime);
    // IDA 0x4c6ef0: reset, release tail storage and five elements.
    static void Cleanup(std::uint32_t object,
                        IInventoryBaseStorageRuntime& runtime);
    // IDA 0x4c6f70: reset five slot ends/counters and the tail end.
    [[nodiscard]] static std::uint32_t Reset(
        std::uint32_t object, IInventoryBaseStorageRuntime& runtime);
};

}  // namespace pal4
