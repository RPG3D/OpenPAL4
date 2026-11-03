#pragma once

#include "pal4/indexed_sorted_value_arrays.h"
#include "pal4/inventory_item_slots.h"
#include <optional>
#include <array>
#include <vector>
#include <unordered_map>

namespace pal4 {
class NativeGameDatabase;
struct NativeGameDbTable;
class NativeBaseObjectMemory;
class NativeInventorySession;

// Typed projections for collection processing, not simulated native DB records.
class NativePlayerCollectionCatalog final {
public:
    explicit NativePlayerCollectionCatalog(const NativeGameDatabase& database);
    NativePlayerCollectionCatalog(const NativeGameDbTable& magic,
        const NativeGameDbTable& stunt, const NativeGameDbTable& skill);
    [[nodiscard]] std::optional<std::uint32_t> MagicSkill(std::uint32_t id) const;
    [[nodiscard]] std::optional<std::uint32_t> StuntSkill(std::uint32_t id) const;
    [[nodiscard]] std::optional<std::uint32_t> SkillDuration(std::uint32_t id) const;
// Original4DE590: Stunt takes precedence over Magic; append permanent
    // attached Skill IDs in five-key order, retaining duplicates.
    [[nodiscard]] std::vector<std::uint32_t> MonsterPassiveSkills(
        const std::array<std::uint32_t,5>& keys) const;
private:
    std::unordered_map<std::uint32_t, std::uint32_t> magic_, stunt_, skill_;
};

class NativePlayerCollectionValues final : public ISortedValueProcessRuntime {
public:
    enum class Kind { Magic, Stunt };
    NativePlayerCollectionValues(const NativePlayerCollectionCatalog& catalog,
        NativeBaseObjectMemory& result_memory, Kind kind)
        : catalog_(catalog), memory_(result_memory), kind_(kind) {}
    std::uint32_t ReadDword(std::uint32_t address) override;
    void Process(std::uint32_t context, std::uint32_t value) override;
private:
    const NativePlayerCollectionCatalog& catalog_;
    NativeBaseObjectMemory& memory_;
    Kind kind_;
};

// Uses the persistent inventory for lookup, and the collection result's memory
// owner for appended passive skill IDs and buffer growth.
class NativePlayerCollectionInventory final : public IInventoryItemSlotsRuntime {
public:
    NativePlayerCollectionInventory(const NativePlayerCollectionCatalog& catalog,
        NativeBaseObjectMemory& result_memory, NativeInventorySession& inventory)
        : catalog_(catalog), memory_(result_memory), inventory_(inventory) {}
    std::uint32_t ReadDword(std::uint32_t address) override;
    std::uint32_t GetGameDatabase() override;
    std::uint32_t ProcessGameDatabaseArray(std::uint32_t array, std::uint32_t value) override;
    std::uint32_t GetInventory() override;
    std::uint32_t FindInventoryEntry(std::uint32_t array, std::uint32_t id, std::uint32_t category) override;
    std::uint32_t ProcessEntry(std::uint32_t first, std::uint32_t second, std::uint32_t entry) override;
    void InsertEntry(std::uint32_t destination, std::uint32_t entry) override;
private:
    const NativePlayerCollectionCatalog& catalog_;
    NativeBaseObjectMemory& memory_;
    NativeInventorySession& inventory_;
};
}  // namespace pal4
