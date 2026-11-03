#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/equipment_window.h"

namespace pal4 {

class IEquipmentCharacterDisplayRuntime {
public:
    virtual ~IEquipmentCharacterDisplayRuntime() = default;
    virtual void SetImage(std::uint32_t window,
                          std::string_view imageset,
                          std::string_view image) = 0;
    [[nodiscard]] virtual std::uint32_t Player(
        std::uint32_t character) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InventoryItem(
        std::uint32_t item_id, std::uint32_t kind) = 0;
    [[nodiscard]] virtual std::string ItemText(
        std::uint32_t address) = 0;
    virtual void SetText(std::uint32_t window,
                         std::string_view text) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
};

class EquipmentCharacterDisplayRuntimeStub :
    public IEquipmentCharacterDisplayRuntime {
public:
    void SetImage(std::uint32_t, std::string_view,
                  std::string_view) override {}
    std::uint32_t Player(std::uint32_t) override { return 0; }
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    std::uint32_t InventoryItem(std::uint32_t,
                                std::uint32_t) override { return 0; }
    std::string ItemText(std::uint32_t) override { return {}; }
    void SetText(std::uint32_t, std::string_view) override {}
    void SetEnabled(std::uint32_t, bool) override {}
};

class EquipmentCharacterDisplay final {
public:
    static void SetCharacterImage(EquipmentWindowState& state,
                                  std::uint32_t character,
                                  IEquipmentCharacterDisplayRuntime& runtime);
    static void UpdateEquipment(EquipmentWindowState& state,
                                 std::uint32_t character,
                                 IEquipmentCharacterDisplayRuntime& runtime);
};

}  // namespace pal4
