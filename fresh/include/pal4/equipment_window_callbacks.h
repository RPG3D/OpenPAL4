#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/equipment_window.h"

namespace pal4 {

class EquipmentCharacterIndex final {
public:
    [[nodiscard]] static std::int32_t Next(std::int32_t current) noexcept;
    [[nodiscard]] static std::int32_t Previous(std::int32_t current) noexcept;
};

// Narrow interfaces let native action owners reuse hover/unequip without
// pretending to implement character/category navigation or model callbacks.
class IEquipmentWindowHoverRuntime {
public:
    virtual ~IEquipmentWindowHoverRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t address)=0;
    virtual std::uint32_t HoveredRow(std::uint32_t list)=0;
    virtual std::uint32_t InventoryItem(std::uint32_t id,std::uint32_t kind)=0;
};
class IEquipmentWindowUnequipRuntime : public IEquipmentWindowHoverRuntime {
public:
    ~IEquipmentWindowUnequipRuntime() override=default;
    virtual void WriteWord(std::uint32_t,std::uint32_t)=0;
    virtual void UpdateList(EquipmentWindowState&,std::uint32_t,std::uint32_t)=0;
    virtual void UpdateDisplay(EquipmentWindowState&,std::uint32_t,std::uint32_t)=0;
    virtual std::uint32_t Player(std::uint32_t)=0;
    virtual void PlaySound(std::string_view,std::uint32_t)=0;
    virtual void RefreshPlayer(std::uint32_t)=0;
    virtual std::uint32_t HoverRowIndex(std::uint32_t)=0;
    virtual float ScrollPosition(std::uint32_t)=0;
    virtual void SetScrollPosition(std::uint32_t,float)=0;
    virtual void SetHoverRowIndex(std::uint32_t,std::uint32_t)=0;
};
class IEquipmentWindowCallbacksRuntime : public IEquipmentWindowUnequipRuntime {
public:
    ~IEquipmentWindowCallbacksRuntime() override=default;
    virtual void HandleItemClick(EquipmentWindowState&,std::uint32_t)=0;
    virtual void UpdateItemDetails(EquipmentWindowState&,std::uint32_t)=0;
    virtual void UpdateItemModel(EquipmentWindowState&,std::uint32_t)=0;
    virtual void SetImage(std::uint32_t,std::uint32_t)=0;
    virtual void InsertMember(std::uint32_t)=0;
    virtual void RemoveMember(std::uint32_t)=0;
    virtual void RefreshSecondaryMember(std::uint32_t)=0;
    virtual std::uint32_t PreviousCharacterIndex(std::uint32_t)=0;
    virtual std::uint32_t NextCharacterIndex(std::uint32_t)=0;
};

class EquipmentWindowCallbacksRuntimeStub :
    public IEquipmentWindowCallbacksRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    std::uint32_t HoveredRow(std::uint32_t) override { return 0; }
    std::uint32_t InventoryItem(std::uint32_t, std::uint32_t) override {
        return 0;
    }
    void HandleItemClick(EquipmentWindowState&, std::uint32_t) override {}
    void UpdateItemDetails(EquipmentWindowState&, std::uint32_t) override {}
    void UpdateItemModel(EquipmentWindowState&, std::uint32_t) override {}
    void UpdateList(EquipmentWindowState&, std::uint32_t,
                    std::uint32_t) override {}
    void UpdateDisplay(EquipmentWindowState&, std::uint32_t,
                       std::uint32_t) override {}
    void SetImage(std::uint32_t, std::uint32_t) override {}
    void InsertMember(std::uint32_t) override {}
    void RemoveMember(std::uint32_t) override {}
    void RefreshSecondaryMember(std::uint32_t) override {}
    std::uint32_t PreviousCharacterIndex(std::uint32_t value) override {
        return static_cast<std::uint32_t>(EquipmentCharacterIndex::Previous(
            static_cast<std::int32_t>(value)));
    }
    std::uint32_t NextCharacterIndex(std::uint32_t value) override {
        return static_cast<std::uint32_t>(EquipmentCharacterIndex::Next(
            static_cast<std::int32_t>(value)));
    }
    std::uint32_t Player(std::uint32_t) override { return 0; }
    void PlaySound(std::string_view, std::uint32_t) override {}
    void RefreshPlayer(std::uint32_t) override {}
    std::uint32_t HoverRowIndex(std::uint32_t) override { return 0; }
    float ScrollPosition(std::uint32_t) override { return 0.0f; }
    void SetScrollPosition(std::uint32_t, float) override {}
    void SetHoverRowIndex(std::uint32_t, std::uint32_t) override {}
};

class EquipmentWindowCallbacks final {
public:
    [[nodiscard]] static std::uint32_t HoveredItem(
        EquipmentWindowState& state,
        IEquipmentWindowHoverRuntime& runtime);
    [[nodiscard]] static bool HandleMouseClick(
        EquipmentWindowState& state, std::uint32_t event,
        IEquipmentWindowCallbacksRuntime& runtime);
    [[nodiscard]] static bool HandleHoverChange(
        EquipmentWindowState& state,
        IEquipmentWindowCallbacksRuntime& runtime);
    [[nodiscard]] static bool HandleCharacterClick(
        EquipmentWindowState& state, std::uint32_t event,
        IEquipmentWindowCallbacksRuntime& runtime);
    [[nodiscard]] static bool SelectCategory(
        EquipmentWindowState& state, std::uint32_t category,
        IEquipmentWindowCallbacksRuntime& runtime);
    [[nodiscard]] static bool SelectPreviousCharacter(
        EquipmentWindowState& state,
        IEquipmentWindowCallbacksRuntime& runtime);
    [[nodiscard]] static bool SelectNextCharacter(
        EquipmentWindowState& state,
        IEquipmentWindowCallbacksRuntime& runtime);
    [[nodiscard]] static bool Unequip(
        EquipmentWindowState& state, std::uint32_t event,
        IEquipmentWindowUnequipRuntime& runtime);
    [[nodiscard]] static bool HandleCharacterSelection(
        EquipmentWindowState& state, std::uint32_t event,
        IEquipmentWindowCallbacksRuntime& runtime);
};

}  // namespace pal4
