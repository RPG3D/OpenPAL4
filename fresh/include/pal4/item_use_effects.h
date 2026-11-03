#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IItemUseEffectsRuntime {
public:
    virtual ~IItemUseEffectsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameDb() = 0;
    [[nodiscard]] virtual std::uint32_t FindRecipe(
        std::uint32_t collection, std::uint32_t recipe) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
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
    [[nodiscard]] virtual std::uint32_t CurrentTime() = 0;
    virtual void SetRandomSeed(std::uint32_t seed) = 0;
    [[nodiscard]] virtual std::int32_t Random() = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(double value) = 0;
    virtual void PlayMusic(std::string_view name, std::uint32_t mode) = 0;
    virtual void UpdateAndClampCounter(std::uint32_t inventory,
                                       std::int32_t delta) = 0;
};

class ItemUseEffects final {
public:
    // IDA 0x5912a0. Returns -2 for unfulfilled materials, -1 for a failed
    // random roll, or zero after applying all item effects.
    static std::int32_t Apply(std::uint32_t object,
                              std::uint32_t recipe,
                              std::uint32_t item,
                              IItemUseEffectsRuntime& runtime);
};

}  // namespace pal4
