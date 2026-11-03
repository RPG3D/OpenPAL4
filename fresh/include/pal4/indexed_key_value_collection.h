#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct KeyValueArraySlot {
    std::uint32_t reserved{};
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t capacity_end{};
};
static_assert(sizeof(KeyValueArraySlot) == 16u);

struct IndexedKeyValueCollectionState {
    std::array<std::uint32_t, 20> prefix{};
    std::array<KeyValueArraySlot, 6> slots{};
};
static_assert(offsetof(IndexedKeyValueCollectionState, slots) == 0x50u);
static_assert(sizeof(IndexedKeyValueCollectionState) == 0xb0u);

class IIndexedKeyValueCollectionRuntime {
public:
    virtual ~IIndexedKeyValueCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IndexedKeyValueCollectionRuntimeStub final
    : public IIndexedKeyValueCollectionRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
};

class IndexedKeyValueCollection final {
public:
    static void AddDelta(IndexedKeyValueCollectionState& state,
                         std::int32_t index, std::int32_t key,
                         std::uint32_t delta,
                         IIndexedKeyValueCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetValue(
        const IndexedKeyValueCollectionState& state,
        std::int32_t index, std::int32_t key,
        IIndexedKeyValueCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t TotalValue(
        const IndexedKeyValueCollectionState& state, std::int32_t index,
        IIndexedKeyValueCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t RemainingValue(
        const IndexedKeyValueCollectionState& state, std::uint32_t input,
        IIndexedKeyValueCollectionRuntime& runtime);
};

}  // namespace pal4
