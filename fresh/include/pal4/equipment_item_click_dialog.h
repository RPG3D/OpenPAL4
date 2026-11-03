#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/equipment_window.h"

namespace pal4 {

struct EquipmentDialogButton {
    std::uint32_t item_id;
    std::string_view action;
    std::string_view text;
    bool enabled;
};

class IEquipmentItemClickRuntime {
public:
    virtual ~IEquipmentItemClickRuntime() = default;
    [[nodiscard]] virtual std::uint32_t HoveredItem(
        EquipmentWindowState& state) = 0;
    [[nodiscard]] virtual std::uint32_t Dialog() = 0;
    virtual void HideAndCleanup(std::uint32_t dialog) = 0;
    [[nodiscard]] virtual std::uint32_t Player(
        std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool ContainsEquipment(
        std::uint32_t player, std::uint32_t item_id) = 0;
    [[nodiscard]] virtual std::int32_t CharacterLevel(
        std::uint32_t player, std::uint32_t field) = 0;
    [[nodiscard]] virtual std::string TriggerText(
        std::uint32_t trigger_id) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    virtual void CreateButton(std::uint32_t dialog,
                              EquipmentDialogButton button) = 0;
    virtual void ShowDialog(std::uint32_t dialog,
                            std::uint32_t event_data) = 0;
    virtual void DestroyCeguiString(std::uint32_t address) = 0;
};

class EquipmentItemClickRuntimeStub : public IEquipmentItemClickRuntime {
public:
    std::uint32_t HoveredItem(EquipmentWindowState&) override { return 0; }
    std::uint32_t Dialog() override { return 0; }
    void HideAndCleanup(std::uint32_t) override {}
    std::uint32_t Player(std::uint32_t) override { return 0; }
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    bool ContainsEquipment(std::uint32_t, std::uint32_t) override {
        return false;
    }
    std::int32_t CharacterLevel(std::uint32_t,
                                std::uint32_t) override { return 0; }
    std::string TriggerText(std::uint32_t) override { return {}; }
    std::string ConvertToUtf8(std::string_view text) override {
        return std::string(text);
    }
    void CreateButton(std::uint32_t, EquipmentDialogButton) override {}
    void ShowDialog(std::uint32_t, std::uint32_t) override {}
    void DestroyCeguiString(std::uint32_t) override {}
};

class EquipmentItemClickDialog final {
public:
    [[nodiscard]] static bool Open(EquipmentWindowState& state,
                                   std::uint32_t event_data,
                                   IEquipmentItemClickRuntime& runtime);
    static void CleanupString(std::uint32_t object,
                              IEquipmentItemClickRuntime& runtime);
};

}  // namespace pal4
