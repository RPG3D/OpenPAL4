#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct CollectionSlotState {
    std::uint32_t reserved{};
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t capacity_end{};
};
static_assert(sizeof(CollectionSlotState) == 16u);
static_assert(offsetof(CollectionSlotState, begin) == 4u);

class ICollectionSlotRuntime {
public:
    virtual ~ICollectionSlotRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    virtual void BeforeFreePairs(std::uint32_t begin,
                                 std::uint32_t end) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class CollectionSlotRuntimeStub final : public ICollectionSlotRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    void BeforeFreePairs(std::uint32_t begin, std::uint32_t end) override;
    void Free(std::uint32_t address) override;
};

class CollectionSlotOperations final {
public:
    [[nodiscard]] static std::uint32_t CopyPairs(
        CollectionSlotState& slot, std::uint32_t destination,
        std::uint32_t source, ICollectionSlotRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReserveWords(
        CollectionSlotState& slot, std::uint32_t count,
        ICollectionSlotRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResizeWords(
        CollectionSlotState& slot, std::uint32_t count,
        std::uint32_t fill, ICollectionSlotRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReservePairs(
        CollectionSlotState& slot, std::uint32_t count,
        ICollectionSlotRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResizePairs(
        CollectionSlotState& slot, std::uint32_t count,
        std::uint32_t first, std::uint32_t second,
        ICollectionSlotRuntime& runtime);
};

class CollectionRecordArray208 final {
public:
    [[nodiscard]] static std::uint32_t Count(const CollectionSlotState& slot);
};

}  // namespace pal4
