#include "pal4/crafting_requirements.h"

#include <array>
#include <cstddef>

namespace pal4 {
namespace {

struct Requirement {
    std::uint32_t item;
    std::uint32_t deficit;
};

std::uint32_t LookupRecipe(const std::uint32_t recipe,
                           ICraftingRequirementRuntime& runtime) {
    return runtime.FindRecipe(runtime.GameDb() + 0x80u, recipe);
}

std::int32_t OwnedCount(const std::uint32_t item,
                        ICraftingRequirementRuntime& runtime,
                        bool& recognized) {
    const auto db = runtime.GameDb();
    if (runtime.IsDirectItem(db, item)) {
        recognized = true;
        return runtime.DirectOwned(item);
    }
    const auto category_db = runtime.GameDb();
    if (runtime.IsCategoryItem(category_db + 0x58u, item)) {
        recognized = true;
        return runtime.CategoryOwned(item);
    }
    recognized = false;
    return 0;
}

}  // namespace

std::uint32_t CraftingRequirements::BuildDeficits(
    const std::uint32_t destination, const std::uint32_t recipe,
    ICraftingRequirementRuntime& runtime) {
    const auto record = LookupRecipe(recipe, runtime);
    std::array<Requirement, 4> deficits{};
    std::size_t used = 0u;

    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto required = static_cast<std::int32_t>(
            runtime.ReadWord(record + 0xa8u + index * 4u));
        const auto item = runtime.ReadWord(record + 0x98u + index * 4u);
        if (required <= 0) continue;

        bool recognized = false;
        const auto owned = OwnedCount(item, runtime, recognized);
        if (recognized && owned < required) {
            deficits[used++] = {
                item, static_cast<std::uint32_t>(required) -
                          static_cast<std::uint32_t>(owned)};
        }
    }

    runtime.WriteByte(destination, static_cast<std::uint8_t>(recipe));
    const auto bytes = static_cast<std::uint32_t>(used * 8u);
    const auto data = runtime.Allocate(bytes);
    runtime.WriteWord(destination + 4u, data);
    for (std::size_t index = 0u; index < used && data != 0u; ++index) {
        runtime.WriteWord(data + static_cast<std::uint32_t>(index * 8u),
                          deficits[index].item);
        runtime.WriteWord(data + static_cast<std::uint32_t>(index * 8u + 4u),
                          deficits[index].deficit);
    }
    runtime.WriteWord(destination + 8u, data + bytes);
    runtime.WriteWord(destination + 12u, data + bytes);
    return destination;
}

bool CraftingRequirements::AreMet(
    const std::uint32_t recipe, ICraftingRequirementRuntime& runtime) {
    const auto record = LookupRecipe(recipe, runtime);
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto required = static_cast<std::int32_t>(
            runtime.ReadWord(record + 0xa8u + index * 4u));
        const auto item = runtime.ReadWord(record + 0x98u + index * 4u);
        if (required <= 0) continue;

        bool recognized = false;
        const auto owned = OwnedCount(item, runtime, recognized);
        if (recognized && owned < required) return false;
    }
    return true;
}

bool CraftingRequirements::HasCategoryItem(
    const std::uint32_t recipe, ICraftingRequirementRuntime& runtime) {
    const auto record = LookupRecipe(recipe, runtime);
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto required = static_cast<std::int32_t>(
            runtime.ReadWord(record + 0xa8u + index * 4u));
        const auto item = runtime.ReadWord(record + 0x98u + index * 4u);
        if (required > 0 && runtime.IsCategoryItem(
                                runtime.GameDb() + 0x58u, item))
            return true;
    }
    return false;
}

std::int32_t CraftingRequirements::FirstCategoryValue(
    const std::uint32_t recipe, ICraftingRequirementRuntime& runtime) {
    const auto record = LookupRecipe(recipe, runtime);
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto required = static_cast<std::int32_t>(
            runtime.ReadWord(record + 0xa8u + index * 4u));
        const auto item = runtime.ReadWord(record + 0x98u + index * 4u);
        if (required > 0 && runtime.IsCategoryItem(
                                runtime.GameDb() + 0x58u, item)) {
            const auto data = runtime.FindCategoryData(
                runtime.GameDb() + 0x58u, item);
            return static_cast<std::int32_t>(
                runtime.ReadWord(data + 0x90u));
        }
    }
    return -1;
}

std::uint32_t CraftingRequirements::GetRecipeProperty(
    const std::uint32_t recipe, ICraftingRequirementRuntime& runtime) {
    return runtime.ReadWord(LookupRecipe(recipe, runtime) + 0x68u);
}

}  // namespace pal4
