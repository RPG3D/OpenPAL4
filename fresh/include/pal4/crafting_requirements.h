#pragma once

#include <cstdint>

namespace pal4 {

class ICraftingRequirementRuntime {
public:
    virtual ~ICraftingRequirementRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameDb() = 0;
    [[nodiscard]] virtual std::uint32_t FindRecipe(
        std::uint32_t collection, std::uint32_t recipe) = 0;
    [[nodiscard]] virtual bool IsDirectItem(std::uint32_t database,
                                             std::uint32_t item) = 0;
    [[nodiscard]] virtual bool IsCategoryItem(std::uint32_t collection,
                                               std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t FindCategoryData(
        std::uint32_t collection, std::uint32_t item) = 0;
    [[nodiscard]] virtual std::int32_t DirectOwned(std::uint32_t item) = 0;
    [[nodiscard]] virtual std::int32_t CategoryOwned(std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
};

class CraftingRequirements final {
public:
    // IDA 0x590b70. Output is a 16-byte tag and pointer-range record.
    static std::uint32_t BuildDeficits(std::uint32_t destination,
                                       std::uint32_t recipe,
                                       ICraftingRequirementRuntime& runtime);
    // IDA 0x590d00.
    static bool AreMet(std::uint32_t recipe,
                       ICraftingRequirementRuntime& runtime);
    // IDA 0x590da0, 0x590e00 and 0x590e70.
    static bool HasCategoryItem(std::uint32_t recipe,
                                ICraftingRequirementRuntime& runtime);
    static std::int32_t FirstCategoryValue(
        std::uint32_t recipe, ICraftingRequirementRuntime& runtime);
    static std::uint32_t GetRecipeProperty(
        std::uint32_t recipe, ICraftingRequirementRuntime& runtime);
};

}  // namespace pal4
