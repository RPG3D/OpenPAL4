#pragma once

#include "pal4/ui_text_buffer_record.h"

#include <cstdint>

namespace pal4 {

class IInventoryItemQueryRuntime : public IUiTextBufferRecordRuntime {
public:
    [[nodiscard]] virtual std::uint32_t InventorySubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryEntry(
        std::uint32_t collection, std::uint32_t key,
        std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t FindGameDbEntry(
        std::uint32_t inventory, std::uint32_t key,
        std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t GameDb() = 0;
    [[nodiscard]] virtual std::uint32_t FindCategory(
        std::uint32_t collection, std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t FindRecipe(
        std::uint32_t collection, std::uint32_t key) = 0;
    [[nodiscard]] virtual std::int32_t ElementCount(
        std::uint32_t collection, std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t ElementAt(
        std::uint32_t collection, std::uint32_t category,
        std::int32_t index) = 0;
};

class InventoryItemQueries final {
public:
    // IDA 0x5908a0.
    static std::uint32_t CopyItemData(std::uint32_t destination,
                                      std::uint32_t key,
                                      IInventoryItemQueryRuntime& runtime);
    // IDA 0x5909f0.
    static std::uint32_t GetProperty(std::uint32_t key,
                                     IInventoryItemQueryRuntime& runtime);
    // IDA 0x590a10.
    static std::uint32_t FindGameDbValue(
        std::uint32_t key, IInventoryItemQueryRuntime& runtime);
    // IDA 0x590a40 and 0x590ac0.
    static std::int32_t CountMatching(std::uint32_t key,
                                      IInventoryItemQueryRuntime& runtime);
    static bool HasQuantityLimit(std::uint32_t key,
                                 IInventoryItemQueryRuntime& runtime);
    // IDA 0x590e90 through 0x590f60.
    static bool IsBelowRecipeQuantity(std::uint32_t recipe,
                                      std::uint32_t item,
                                      IInventoryItemQueryRuntime& runtime);
    static bool MeetsRecipeProperty(std::uint32_t recipe,
                                    std::uint32_t item,
                                    IInventoryItemQueryRuntime& runtime);
    static bool HasItemProperty(std::uint32_t item,
                                IInventoryItemQueryRuntime& runtime);
    static bool MatchesRecipeProperty(std::uint32_t item,
                                      std::uint32_t recipe,
                                      IInventoryItemQueryRuntime& runtime);
};

}  // namespace pal4
