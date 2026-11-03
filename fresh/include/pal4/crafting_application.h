#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICraftingApplicationRuntime {
public:
    virtual ~ICraftingApplicationRuntime() = default;
    virtual void PlayMusic(std::string_view name, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t GameDb() = 0;
    [[nodiscard]] virtual std::uint32_t FindRecipe(
        std::uint32_t collection, std::uint32_t recipe) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool IsDirectItem(
        std::uint32_t database, std::uint32_t item) = 0;
    virtual void FindOrLogDirectItem(std::uint32_t database,
                                     std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    virtual void RemoveDirectItem(std::uint32_t inventory,
                                  std::uint32_t item,
                                  std::uint32_t quantity) = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryEntry(
        std::uint32_t collection, std::uint32_t key,
        std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessGameDataCollection(
        std::uint32_t collection, std::uint32_t property) = 0;
    [[nodiscard]] virtual bool CheckPlayerCondition(
        std::uint32_t entry) = 0;
    virtual void ManagePlayerItem(std::uint32_t object,
                                  std::uint32_t item,
                                  std::uint32_t result_item) = 0;
    virtual void RemoveInventoryEntry(std::uint32_t collection,
                                      std::uint32_t entry) = 0;
    virtual void UpdateAndClampCounter(std::uint32_t inventory,
                                       std::int32_t delta) = 0;
};

class CraftingApplication final {
public:
    // IDA 0x590fb0.
    static std::int32_t Apply(std::uint32_t object,
                              std::uint32_t recipe,
                              std::uint32_t selected_item,
                              ICraftingApplicationRuntime& runtime);
};

}  // namespace pal4
