#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/equipment_window.h"
#include "pal4/equipment_level_formatter.h"

namespace pal4 {

class IEquipmentWindowMutationRuntime {
public:
    virtual ~IEquipmentWindowMutationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Player(std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t InventoryItem(std::uint32_t id,
                                                       std::uint32_t kind) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void PlaySound(std::string_view file, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t EquipmentSlot(std::uint32_t player,
                                                       std::uint32_t item) = 0;
    virtual void RefreshPlayer(std::uint32_t player) = 0;
    virtual void UpdateList(EquipmentWindowState& state,
                            std::uint32_t category,
                            std::uint32_t character) = 0;
    virtual void UpdateDisplay(EquipmentWindowState& state,
                               std::uint32_t item,
                               std::uint32_t character) = 0;
    [[nodiscard]] virtual std::uint32_t HoveredItem(
        EquipmentWindowState& state) = 0;
    virtual void RemoveInventoryItem(std::uint32_t item) = 0;
    [[nodiscard]] virtual std::string ItemInfoFormat(std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string ItemText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string FormatItemInfo(
        std::string_view format, std::string_view item_text) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::uint32_t bytes) = 0;
    virtual void InitializeDiscardCallback(std::uint32_t callback,
                                            std::uint32_t owner) = 0;
    virtual void ShowCommonDialog(std::string_view text,
                                  float x, float y, std::uint32_t style,
                                  std::uint32_t callback) = 0;
    virtual void ReleaseCallback(std::uint32_t callback,
                                 std::uint32_t flags) = 0;
};

class IEquipmentWindowPresentationRuntime {
public:
    virtual ~IEquipmentWindowPresentationRuntime() = default;
    virtual std::uint32_t Player(std::uint32_t index)=0;
    virtual void ClearItemDetails(EquipmentWindowState& state) = 0;
    virtual void SetEquipmentVisibility(EquipmentWindowState& state,
                                        bool visible) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t CharacterLevel(
        std::uint32_t player, std::uint32_t field) = 0;
    [[nodiscard]] virtual std::string FormatLevel(
        std::uint32_t level) = 0;
    virtual void SetText(std::uint32_t window, std::string_view text) = 0;
    virtual void SetSortDirection(std::uint32_t list,
                                  std::uint32_t direction) = 0;
    virtual void ResetList(std::uint32_t list) = 0;
    virtual void PopulateList(EquipmentWindowState& state,
                              std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    virtual void SetHoverRow(std::uint32_t list, std::uint32_t row) = 0;
    virtual void SetCharacterImage(EquipmentWindowState& state,
                                   std::uint32_t character) = 0;
    virtual void SetSelected(std::uint32_t button, bool selected) = 0;
    virtual void UpdateCharacterEquipment(EquipmentWindowState& state,
                                           std::uint32_t character) = 0;
    virtual void UpdateItemDetails(EquipmentWindowState& state,
                                   std::uint32_t item) = 0;
};
class IEquipmentWindowActionsRuntime : public IEquipmentWindowMutationRuntime,
    public IEquipmentWindowPresentationRuntime {
public:
    ~IEquipmentWindowActionsRuntime() override=default;
    virtual std::uint32_t Player(std::uint32_t index) override=0;
};

class EquipmentWindowActionsRuntimeStub :
    public IEquipmentWindowActionsRuntime {
public:
    std::uint32_t Player(std::uint32_t) override { return 0; }
    std::uint32_t InventoryItem(std::uint32_t, std::uint32_t) override {
        return 0;
    }
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void PlaySound(std::string_view, std::uint32_t) override {}
    std::uint32_t EquipmentSlot(std::uint32_t, std::uint32_t) override {
        return 6;
    }
    void RefreshPlayer(std::uint32_t) override {}
    void UpdateList(EquipmentWindowState&, std::uint32_t,
                    std::uint32_t) override {}
    void UpdateDisplay(EquipmentWindowState&, std::uint32_t,
                       std::uint32_t) override {}
    std::uint32_t HoveredItem(EquipmentWindowState&) override { return 0; }
    void RemoveInventoryItem(std::uint32_t) override {}
    std::string ItemInfoFormat(std::uint32_t) override { return {}; }
    std::string ItemText(std::uint32_t) override { return {}; }
    std::string FormatItemInfo(std::string_view,
                               std::string_view) override { return {}; }
    std::uint32_t AllocateCallback(std::uint32_t) override { return 0; }
    void InitializeDiscardCallback(std::uint32_t, std::uint32_t) override {}
    void ShowCommonDialog(std::string_view, float, float,
                          std::uint32_t, std::uint32_t) override {}
    void ReleaseCallback(std::uint32_t, std::uint32_t) override {}
    void ClearItemDetails(EquipmentWindowState&) override {}
    void SetEquipmentVisibility(EquipmentWindowState&, bool) override {}
    void SetVisible(std::uint32_t, bool) override {}
    std::uint32_t CharacterLevel(std::uint32_t, std::uint32_t) override {
        return 0;
    }
    std::string FormatLevel(std::uint32_t level) override {
        return EquipmentLevelFormatter::Format(
            static_cast<std::int32_t>(level));
    }
    void SetText(std::uint32_t, std::string_view) override {}
    void SetSortDirection(std::uint32_t, std::uint32_t) override {}
    void ResetList(std::uint32_t) override {}
    void PopulateList(EquipmentWindowState&, std::uint32_t) override {}
    std::uint32_t RowCount(std::uint32_t) override { return 0; }
    void SetHoverRow(std::uint32_t, std::uint32_t) override {}
    void SetCharacterImage(EquipmentWindowState&, std::uint32_t) override {}
    void SetSelected(std::uint32_t, bool) override {}
    void UpdateCharacterEquipment(EquipmentWindowState&,
                                   std::uint32_t) override {}
    void UpdateItemDetails(EquipmentWindowState&, std::uint32_t) override {}
};

class EquipmentWindowActions final {
public:
    [[nodiscard]] static bool Equip(EquipmentWindowState& state,
                                    std::uint32_t item_id,
                                    IEquipmentWindowMutationRuntime& runtime);
    [[nodiscard]] static bool ShowDiscardConfirmation(
        EquipmentWindowState& state, std::uint32_t object_address,
        std::uint32_t item_id, IEquipmentWindowMutationRuntime& runtime);
    [[nodiscard]] static bool ConfirmDiscard(EquipmentWindowState& state,
                                      std::uint32_t answer,
                                      IEquipmentWindowMutationRuntime& runtime);
    static void UpdateList(EquipmentWindowState& state,
                           std::uint32_t category, std::uint32_t character,
                           IEquipmentWindowPresentationRuntime& runtime);
    static void UpdateDisplay(EquipmentWindowState& state,
                              std::uint32_t item, std::uint32_t character,
                              IEquipmentWindowPresentationRuntime& runtime);
};

}  // namespace pal4
