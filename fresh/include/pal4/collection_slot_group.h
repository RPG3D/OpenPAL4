#pragma once

#include <cstdint>

namespace pal4 {

class ICollectionSlotGroupRuntime {
public:
    virtual ~ICollectionSlotGroupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void DestroyEntry(std::uint32_t entry) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    virtual void ReportInvalidSlot(std::int32_t index,
                                   std::uint32_t source_line) = 0;
};

class ICollectionSlotGroupProcessRuntime : public ICollectionSlotGroupRuntime {
public:
    [[nodiscard]] virtual std::uint32_t FindGameData(std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void CopyItemSnapshot(std::uint32_t destination,
                                  std::uint32_t source) = 0;
    virtual void AppendToSlot(std::uint32_t slot,
                              std::uint32_t snapshot) = 0;
    virtual void InsertGlobalData(std::uint32_t key,
                                  std::uint32_t snapshot) = 0;
};

class CollectionSlotGroup final {
public:
    static constexpr std::uint32_t kVtable = 0x84346cu;
    static constexpr std::uint32_t kBaseVtable = 0x84347cu;

    // IDA 0x4c42e0. seed is the copied uninitialized stack byte.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint8_t seed,
        std::uint8_t element_seed, ICollectionSlotGroupRuntime& runtime);
    // IDA 0x4c4360, a single 16-byte element.
    static void InitializeElement(std::uint32_t element, std::uint8_t seed,
                                  ICollectionSlotGroupRuntime& runtime);
    // IDA 0x4c4380: destroy entries, tail buffer, then seven elements.
    static void Destroy(std::uint32_t object,
                        ICollectionSlotGroupRuntime& runtime);
    // IDA 0x4c4400: returns the old tail end pointer.
    [[nodiscard]] static std::uint32_t ClearEntries(
        std::uint32_t object, ICollectionSlotGroupRuntime& runtime);
    // IDA 0x4c44a0: adds a game-data snapshot and records its sequence ID.
    [[nodiscard]] static std::uint32_t ProcessGameData(
        std::uint32_t object, std::uint32_t key,
        ICollectionSlotGroupProcessRuntime& runtime);
    // IDA 0x4c4740: remove by snapshot sequence, then compact the slot.
    [[nodiscard]] static bool RemoveGameData(
        std::uint32_t object, std::uint32_t snapshot,
        ICollectionSlotGroupProcessRuntime& runtime);
    // IDA 0x4c4840: invalid index scans all seven slots.
    [[nodiscard]] static std::uint32_t FindSequence(
        std::uint32_t object, std::int32_t sequence,
        std::int32_t index, ICollectionSlotGroupRuntime& runtime);
    // IDA 0x4c48a0/0x4c48d0.
    [[nodiscard]] static std::uint32_t CountInSlot(
        std::uint32_t object, std::int32_t index,
        ICollectionSlotGroupRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetEntry(
        std::uint32_t object, std::int32_t index,
        std::uint32_t offset, ICollectionSlotGroupRuntime& runtime);
    // IDA 0x4c4910/0x4c4960.
    [[nodiscard]] static std::uint32_t SlotAddress(
        std::uint32_t object, std::int32_t index,
        ICollectionSlotGroupRuntime& runtime);
    [[nodiscard]] static std::uint32_t SearchSlot(
        std::uint32_t slot, std::int32_t sequence,
        ICollectionSlotGroupRuntime& runtime);
    // IDA 0x4c5bf0. Returns the first pointer slot with key >= sequence.
    [[nodiscard]] static std::uint32_t LowerBoundSequence(
        std::uint32_t begin, std::uint32_t end,
        std::int32_t sequence, ICollectionSlotGroupRuntime& runtime);
};

}  // namespace pal4
