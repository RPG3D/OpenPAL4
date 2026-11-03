#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct InventoryItemSlotsState {
    std::array<std::uint32_t, 6> ids{};
};
static_assert(sizeof(InventoryItemSlotsState) == 24u);

class IInventoryItemSlotsRuntime {
public:
    virtual ~IInventoryItemSlotsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetGameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessGameDatabaseArray(
        std::uint32_t array_address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GetInventory() = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryEntry(
        std::uint32_t array_address, std::uint32_t id,
        std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessEntry(
        std::uint32_t first_argument, std::uint32_t second_argument,
        std::uint32_t entry) = 0;
    virtual void InsertEntry(std::uint32_t destination,
                             std::uint32_t entry) = 0;
};

class InventoryItemSlotsRuntimeStub final : public IInventoryItemSlotsRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t address) override;
    std::uint32_t GetGameDatabase() override;
    std::uint32_t ProcessGameDatabaseArray(
        std::uint32_t array_address, std::uint32_t value) override;
    std::uint32_t GetInventory() override;
    std::uint32_t FindInventoryEntry(
        std::uint32_t array_address, std::uint32_t id,
        std::uint32_t category) override;
    std::uint32_t ProcessEntry(std::uint32_t first_argument,
                               std::uint32_t second_argument,
                               std::uint32_t entry) override;
    void InsertEntry(std::uint32_t destination,
                     std::uint32_t entry) override;
};

class InventoryItemSlots final {
public:
    [[nodiscard]] static std::uint32_t Initialize(
        InventoryItemSlotsState& state);
    [[nodiscard]] static std::uint32_t Classify(
        const InventoryItemSlotsState& state,
        std::uint32_t record_address,
        IInventoryItemSlotsRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindSlotOrClassify(
        const InventoryItemSlotsState& state,
        std::uint32_t record_address,
        IInventoryItemSlotsRuntime& runtime);
    [[nodiscard]] static bool Contains(
        const InventoryItemSlotsState& state, std::uint32_t id);
    [[nodiscard]] static std::uint32_t GetIndexed(
        const InventoryItemSlotsState& state, std::int32_t index);
    [[nodiscard]] static std::uint32_t SetIndexed(
        InventoryItemSlotsState& state, std::int32_t index,
        std::uint32_t value);
    [[nodiscard]] static std::uint32_t ProcessEach(
        const InventoryItemSlotsState& state,
        std::uint32_t first_argument,
        std::uint32_t second_argument,
        IInventoryItemSlotsRuntime& runtime);
    static void ProcessAndInsertEach(
        const InventoryItemSlotsState& state, std::uint32_t destination,
        IInventoryItemSlotsRuntime& runtime);
};

}  // namespace pal4
