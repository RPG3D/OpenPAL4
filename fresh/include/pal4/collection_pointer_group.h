#pragma once

#include "pal4/collection_pointer_array.h"

#include <cstdint>

namespace pal4 {

struct CollectionPointerGroupState {
    std::uint32_t vtable{};
    std::uint32_t first_begin{};
    std::uint32_t first_end{};
    std::uint32_t first_capacity{};
    std::uint32_t unknown_10{};
    std::uint32_t second_begin{};
    std::uint32_t second_end{};
    std::uint32_t second_capacity{};
    std::uint32_t unknown_20{};
    std::uint32_t third_begin{};
    std::uint32_t third_end{};
    std::uint32_t third_capacity{};
};
static_assert(sizeof(CollectionPointerGroupState) == 48u);

class ICollectionPointerGroupRuntime {
public:
    virtual ~ICollectionPointerGroupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FreeMemory(
        std::uint32_t address) = 0;
    virtual void DeleteObject(std::uint32_t address,
                              std::uint32_t flag) = 0;
};

class CollectionPointerGroupRuntimeStub final
    : public ICollectionPointerGroupRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    std::uint32_t FreeMemory(std::uint32_t address) override;
    void DeleteObject(std::uint32_t address,
                      std::uint32_t flag) override;
};

struct CollectionPointerVisitorState {
    std::uint32_t vtable{0x849718u};
    std::uint32_t input{};
    std::uint32_t output{};
};
static_assert(sizeof(CollectionPointerVisitorState) == 12u);

class ICollectionProcessAllRuntime : public ICollectionPointerArrayRuntime {
public:
    [[nodiscard]] virtual std::uint32_t CreateIterator(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t IsEnd(
        std::uint32_t iterator, std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual std::uint32_t Current(
        std::uint32_t iterator, std::uint32_t vtable_offset) = 0;
    virtual void ProcessCurrent(std::uint32_t element,
                                CollectionPointerVisitorState& visitor,
                                std::uint32_t vtable_offset) = 0;
    virtual void Advance(std::uint32_t iterator,
                         std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual std::int32_t DestroyIterator(
        std::uint32_t iterator, std::uint32_t flag) = 0;
};

class ICollectionPointerSortRuntime {
public:
    virtual ~ICollectionPointerSortRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    virtual void InsertionSort(std::uint32_t begin, std::uint32_t end,
                               std::uint32_t sentinel,
                               std::uint32_t original_end) = 0;
    [[nodiscard]] virtual std::uint32_t CompareThree(
        std::uint32_t first, std::uint32_t middle,
        std::uint32_t last, std::uint32_t original_end) = 0;
    [[nodiscard]] virtual std::uint32_t Partition(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t pivot) = 0;
    virtual void PartitionRange(std::uint32_t begin, std::uint32_t end,
                                std::uint32_t original_end) = 0;
    [[nodiscard]] virtual std::int32_t CompareItems(
        std::uint32_t candidate, std::uint32_t previous) = 0;
};

class CollectionPointerSortRuntimeStub final
    : public ICollectionPointerSortRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t AllocateMemory(std::uint32_t bytes) override;
    void FreeMemory(std::uint32_t address) override;
    void InsertionSort(std::uint32_t begin, std::uint32_t end,
                       std::uint32_t sentinel,
                       std::uint32_t original_end) override;
    std::uint32_t CompareThree(std::uint32_t first,
                                std::uint32_t middle,
                                std::uint32_t last,
                                std::uint32_t original_end) override;
    std::uint32_t Partition(std::uint32_t begin, std::uint32_t end,
                            std::uint32_t pivot) override;
    void PartitionRange(std::uint32_t begin, std::uint32_t end,
                        std::uint32_t original_end) override;
    std::int32_t CompareItems(std::uint32_t candidate,
                              std::uint32_t previous) override;
};

class ICollectionNameLookupRuntime {
public:
    virtual ~ICollectionNameLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetNameRecord(
        std::uint32_t object, std::uint32_t vtable_offset) = 0;
};

class CollectionNameLookupRuntimeStub final
    : public ICollectionNameLookupRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    std::uint32_t GetNameRecord(std::uint32_t object,
                                std::uint32_t vtable_offset) override;
};

class CollectionPointerGroup final {
public:
    // IDA 0x5ee2a0 / 0x5ee320 / 0x5ee3f0.
    [[nodiscard]] static std::uint32_t Cleanup(
        CollectionPointerGroupState& group,
        ICollectionPointerGroupRuntime& runtime);
    [[nodiscard]] static std::uint32_t Clear(
        CollectionPointerGroupState& group,
        ICollectionPointerGroupRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupStringHolder(
        std::uint32_t holder, ICollectionPointerGroupRuntime& runtime);
    // IDA 0x5ee430.
    [[nodiscard]] static std::int32_t ProcessAll(
        CollectionPointerGroupState& group, std::uint32_t source,
        std::uint32_t input, ICollectionProcessAllRuntime& runtime);
    // IDA 0x5ee6d0.
    static void Sort(CollectionPointerGroupState& group,
                     ICollectionPointerSortRuntime& runtime);
    // IDA 0x5ee900.
    [[nodiscard]] static std::uint32_t FindByName(
        const CollectionPointerGroupState& group, const char* name,
        ICollectionNameLookupRuntime& runtime);

private:
    [[nodiscard]] static std::int32_t CompareName(
        std::uint32_t record, const char* name,
        ICollectionNameLookupRuntime& runtime);
};

}  // namespace pal4
