#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/equipment_window.h"

namespace pal4 {

class IEquipmentWindowVisibilityRuntime {
public:
    virtual ~IEquipmentWindowVisibilityRuntime() = default;
    virtual void SetParent(EquipmentWindowState& state,
                           std::uint8_t showing, std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void ProcessEvent(std::uint32_t tree,
                              std::string_view command,
                              std::uint32_t first,
                              std::uint32_t second) = 0;
    virtual void SetEquipmentVisibility(EquipmentWindowState& state,
                                        bool visible) = 0;
    virtual void ClearItemDetails(EquipmentWindowState& state) = 0;
    virtual void SetSelected(std::uint32_t window, bool selected) = 0;
    virtual void SetText(std::uint32_t window,
                         std::u32string_view text) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t NextCharacterIndex(
        std::uint32_t current) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void InsertActive(std::uint32_t array,
                              std::uint32_t member) = 0;
    virtual void RemoveActive(std::uint32_t array,
                              std::uint32_t member) = 0;
    virtual void SelectCharacter(EquipmentWindowState& state,
                                 std::uint32_t value) = 0;
    virtual void UpdateSystemDialogVisibility(std::uint32_t manager,
                                              std::uint32_t dialog) = 0;
    virtual void ToggleModelVisibility(EquipmentWindowState& state,
                                       bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t RefreshMember(
        std::uint32_t member) = 0;
};

class EquipmentWindowVisibilityRuntimeStub :
    public IEquipmentWindowVisibilityRuntime {
public:
    void SetParent(EquipmentWindowState&, std::uint8_t,
                   std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    void ProcessEvent(std::uint32_t, std::string_view,
                      std::uint32_t, std::uint32_t) override {}
    void SetEquipmentVisibility(EquipmentWindowState&, bool) override {}
    void ClearItemDetails(EquipmentWindowState&) override {}
    void SetSelected(std::uint32_t, bool) override {}
    void SetText(std::uint32_t, std::u32string_view) override {}
    void SetEnabled(std::uint32_t, bool) override {}
    [[nodiscard]] std::uint32_t NextCharacterIndex(
        std::uint32_t) override { return 4u; }
    [[nodiscard]] std::uint32_t PlayerByIndex(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    void InsertActive(std::uint32_t, std::uint32_t) override {}
    void RemoveActive(std::uint32_t, std::uint32_t) override {}
    void SelectCharacter(EquipmentWindowState&, std::uint32_t) override {}
    void UpdateSystemDialogVisibility(std::uint32_t,
                                      std::uint32_t) override {}
    void ToggleModelVisibility(EquipmentWindowState&, bool) override {}
    [[nodiscard]] std::uint32_t RefreshMember(
        std::uint32_t) override { return 0u; }
};

class EquipmentWindowVisibility final {
public:
    [[nodiscard]] static std::uint32_t SetVisible(
        EquipmentWindowState& state, std::uint32_t object_address,
        std::uint8_t showing, std::uint32_t parent,
        IEquipmentWindowVisibilityRuntime& runtime);
};

}  // namespace pal4
