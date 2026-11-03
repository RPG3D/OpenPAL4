#pragma once

#include <cstdint>

#include "pal4/equipment_window.h"

namespace pal4 {

struct EquipmentCategoryMemberState {
    std::uint32_t selected_index{};
};

class IEquipmentCategoryEventsRuntime {
public:
    virtual ~IEquipmentCategoryEventsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ControlWindow(
        EquipmentWindowState& state, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t NavigationWindow(
        EquipmentWindowState& state, std::uint32_t index) = 0;
    [[nodiscard]] virtual bool IsDisabled(std::uint32_t window) = 0;
    virtual void SetHoverFlag(std::uint32_t window, bool hovered) = 0;
    virtual void RequestRedraw(std::uint32_t window) = 0;
    virtual void RemoveCategoryMember() = 0;
    virtual void RemovePrimaryMember() = 0;
    virtual void InsertPrimaryMember() = 0;
    virtual void SetEquipmentVisible(EquipmentWindowState& state,
                                     bool visible) = 0;
    virtual void HandleMouseEvent(std::uint32_t window) = 0;
    [[nodiscard]] virtual bool HandleBaseKey(
        EquipmentCategoryMemberState& member,
        std::uint32_t event) = 0;
};

class EquipmentCategoryEventsRuntimeStub :
    public IEquipmentCategoryEventsRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    std::uint32_t ControlWindow(EquipmentWindowState& state,
                                std::uint32_t index) override {
        return state.Read(0x98u + 4u * index);
    }
    std::uint32_t NavigationWindow(EquipmentWindowState& state,std::uint32_t index) override {
        constexpr std::uint32_t order[]{0u,1u,4u,3u,2u,5u};
        return index<6u?state.Read(0x98u+4u*order[index]):0u;
    }
    bool IsDisabled(std::uint32_t) override { return true; }
    void SetHoverFlag(std::uint32_t, bool) override {}
    void RequestRedraw(std::uint32_t) override {}
    void RemoveCategoryMember() override {}
    void RemovePrimaryMember() override {}
    void InsertPrimaryMember() override {}
    void SetEquipmentVisible(EquipmentWindowState&, bool) override {}
    void HandleMouseEvent(std::uint32_t) override {}
    bool HandleBaseKey(EquipmentCategoryMemberState&,
                       std::uint32_t) override { return false; }
};

class EquipmentCategoryEvents final {
public:
    [[nodiscard]] static bool Handle(EquipmentWindowState& state,
                                     EquipmentCategoryMemberState& member,
                                     std::uint32_t event,
                                     IEquipmentCategoryEventsRuntime& runtime);
    [[nodiscard]] static bool SelectFirstEnabled(
        EquipmentWindowState& state,
        EquipmentCategoryMemberState& member,
        IEquipmentCategoryEventsRuntime& runtime);
    [[nodiscard]] static bool ClearHighlight(
        EquipmentWindowState& state,
        IEquipmentCategoryEventsRuntime& runtime);
};

}  // namespace pal4
