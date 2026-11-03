#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct SortedValueArraySlot {
    std::uint32_t reserved{};
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t capacity_end{};
};
static_assert(sizeof(SortedValueArraySlot) == 16u);
static_assert(offsetof(SortedValueArraySlot, begin) == 4u);
static_assert(offsetof(SortedValueArraySlot, end) == 8u);
static_assert(offsetof(SortedValueArraySlot, capacity_end) == 12u);

struct IndexedSortedValueArrayState {
    std::array<SortedValueArraySlot, 6> slots{};
};
static_assert(sizeof(IndexedSortedValueArrayState) == 96u);

class IIndexedSortedValueArrayRuntime {
public:
    virtual ~IIndexedSortedValueArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IndexedSortedValueArrayRuntimeStub final
    : public IIndexedSortedValueArrayRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
};

struct IndexedSortedIteratorImage {
    std::uint32_t vtable{};
    std::uint32_t current{};
    std::uint32_t owner{};
};
static_assert(sizeof(IndexedSortedIteratorImage) == 12u);

struct InvalidSortedIteratorImage {
    std::uint32_t vtable{};
    std::uint32_t index{};
};
static_assert(sizeof(InvalidSortedIteratorImage) == 8u);

class IIndexedSortedIteratorRuntime {
public:
    virtual ~IIndexedSortedIteratorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void StoreValid(std::uint32_t address,
                            const IndexedSortedIteratorImage& image) = 0;
    virtual void StoreInvalid(std::uint32_t address,
                              const InvalidSortedIteratorImage& image) = 0;
    virtual void WriteOutput(std::uint32_t address, std::uint32_t value) = 0;
};

class IndexedSortedIteratorRuntimeStub final
    : public IIndexedSortedIteratorRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override;
    void StoreValid(std::uint32_t address,
                    const IndexedSortedIteratorImage& image) override;
    void StoreInvalid(std::uint32_t address,
                      const InvalidSortedIteratorImage& image) override;
    void WriteOutput(std::uint32_t address, std::uint32_t value) override;
};

class ISortedValueProcessRuntime {
public:
    virtual ~ISortedValueProcessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void Process(std::uint32_t context, std::uint32_t value) = 0;
};

class SortedValueProcessRuntimeStub final : public ISortedValueProcessRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t address) override;
    void Process(std::uint32_t context, std::uint32_t value) override;
};

class IndexedSortedValueArrays final {
public:
    static constexpr std::uint32_t kValidIteratorVtable = 0x842bfcu;
    static constexpr std::uint32_t kInvalidIteratorVtable = 0x842c28u;
    [[nodiscard]] static std::uint32_t InsertUnique(
        IndexedSortedValueArrayState& state, std::int32_t index,
        std::int32_t value, IIndexedSortedValueArrayRuntime& runtime);
    [[nodiscard]] static bool Contains(
        const IndexedSortedValueArrayState& state, std::int32_t index,
        std::int32_t value, IIndexedSortedValueArrayRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetIteratorByIndex(
        const IndexedSortedValueArrayState& state, std::uint32_t object_address,
        std::uint32_t output_address, std::int32_t index,
        IIndexedSortedIteratorRuntime& runtime);
};

struct IndexedFilteredResultArrayState {
    std::uint32_t reserved{};
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t capacity_end{};
};
static_assert(sizeof(IndexedFilteredResultArrayState) == 16u);

class IDynamicDwordArrayRuntime {
public:
    virtual ~IDynamicDwordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class DynamicDwordResultArray final {
public:
    [[nodiscard]] static std::uint32_t Append(
        IndexedFilteredResultArrayState& result, std::uint32_t value,
        IDynamicDwordArrayRuntime& runtime);
};

class IIndexedFilteredValueRuntime : public IDynamicDwordArrayRuntime {
public:
    ~IIndexedFilteredValueRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t GetGameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t LookupRecord(
        std::uint32_t collection_address, std::uint32_t key) = 0;
};

class IndexedFilteredValueRuntimeStub final
    : public IIndexedFilteredValueRuntime {
public:
    std::uint32_t GetGameDatabase() override;
    std::uint32_t LookupRecord(std::uint32_t collection_address,
                               std::uint32_t key) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
};

class IndexedFilteredValueCollector final {
public:
    [[nodiscard]] static std::uint32_t Collect(
        const IndexedSortedValueArrayState& source,
        IndexedFilteredResultArrayState& result, std::uint32_t filter,
        IIndexedFilteredValueRuntime& runtime);
};

struct FixedSortedValueArrayState {
    std::array<std::uint32_t, 40> prefix{};
    SortedValueArraySlot values{};
};
static_assert(offsetof(FixedSortedValueArrayState, values) == 0xa0u);
static_assert(sizeof(FixedSortedValueArrayState) == 0xb0u);

class FixedSortedValueArray final {
public:
    static void InsertUnique(FixedSortedValueArrayState& state,
                             std::int32_t value,
                             IIndexedSortedValueArrayRuntime& runtime);
    [[nodiscard]] static bool Contains(
        const FixedSortedValueArrayState& state, std::int32_t value,
        IIndexedSortedValueArrayRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetIterator(
        const FixedSortedValueArrayState& state, std::uint32_t object_address,
        std::uint32_t output_address, IIndexedSortedIteratorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Clear(FixedSortedValueArrayState& state);
    [[nodiscard]] static std::uint32_t ProcessValues(
        const FixedSortedValueArrayState& state, std::uint32_t context,
        ISortedValueProcessRuntime& runtime);
};

class FixedFilteredValueCollector final {
public:
    [[nodiscard]] static std::uint32_t Collect(
        const FixedSortedValueArrayState& source,
        IndexedFilteredResultArrayState& result, std::uint32_t filter,
        IIndexedFilteredValueRuntime& runtime);
};

}  // namespace pal4
