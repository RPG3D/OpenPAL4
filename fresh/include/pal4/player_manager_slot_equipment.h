#pragma once

#include <cstdint>

namespace pal4 {

struct PlayerEquipmentVisuals {
    std::uint32_t primary{};
    std::uint32_t secondary{};
    std::uint32_t shared_effect{};
    std::uint32_t extra{};
    std::uint32_t trailing{};
};

class IPlayerManagerSlotEquipmentRuntime {
public:
    virtual ~IPlayerManagerSlotEquipmentRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InventoryManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindItem(
        std::uint32_t collection, std::uint32_t key,
        std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t CreateName(
        std::uint32_t source_text) = 0;
    [[nodiscard]] virtual bool NameEquals(
        std::uint32_t name, const char* expected) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupName(
        std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual std::uint32_t SharedEffect() = 0;
    [[nodiscard]] virtual std::uint32_t IndexedText(
        std::uint32_t item, std::uint32_t index) = 0;
    virtual void ApplyVisuals(std::uint32_t object,
                              const PlayerEquipmentVisuals& visuals) = 0;
    virtual void ApplyWeapon(std::uint32_t object,
                             const PlayerEquipmentVisuals& visuals) = 0;
};

class PlayerManagerSlotEquipmentRuntimeStub :
    public IPlayerManagerSlotEquipmentRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    std::uint32_t InventoryManager() override;
    std::uint32_t FindItem(std::uint32_t, std::uint32_t,
                           std::uint32_t) override;
    std::uint32_t CreateName(std::uint32_t) override;
    bool NameEquals(std::uint32_t, const char*) override;
    std::uint32_t CleanupName(std::uint32_t) override;
    std::uint32_t EmptyText() override;
    std::uint32_t SharedEffect() override;
    std::uint32_t IndexedText(std::uint32_t,
                               std::uint32_t) override;
    void ApplyVisuals(std::uint32_t,
                      const PlayerEquipmentVisuals&) override;
    void ApplyWeapon(std::uint32_t,
                     const PlayerEquipmentVisuals&) override;
};

class PlayerManagerSlotEquipment final {
public:
    // IDA 0x43eeb0: choose per-character equipment visuals from inventory.
    [[nodiscard]] static std::uint32_t Apply(
        std::uint32_t object, IPlayerManagerSlotEquipmentRuntime& runtime);
};

}  // namespace pal4
