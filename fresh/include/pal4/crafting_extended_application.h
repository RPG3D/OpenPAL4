#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICraftingExtendedRuntime {
public:
    virtual ~ICraftingExtendedRuntime() = default;
    virtual void PlayMusic(std::string_view name, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t GameDb() = 0;
    [[nodiscard]] virtual std::uint32_t FindRecipe(
        std::uint32_t collection, std::uint32_t recipe) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
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
    [[nodiscard]] virtual std::uint32_t EmptyCString() = 0;
    [[nodiscard]] virtual std::uint32_t CStringLength(
        std::uint32_t source) = 0;
    virtual void AssignCString(std::uint32_t destination,
                               std::uint32_t source,
                               std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t CreateTemporaryText(
        std::uint8_t initial_mode) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source,
                            std::uint32_t start,
                            std::uint32_t count) = 0;
    virtual void ReleaseTemporaryText(std::uint32_t temporary) = 0;
    virtual void UpdateAndClampCounter(std::uint32_t inventory,
                                       std::int32_t delta) = 0;
};

class CraftingExtendedApplication final {
public:
    // IDA 0x5920e0.
    static std::int32_t Apply(std::uint32_t object,
                              std::uint32_t recipe,
                              std::uint32_t target_item,
                              ICraftingExtendedRuntime& runtime);
};

}  // namespace pal4
