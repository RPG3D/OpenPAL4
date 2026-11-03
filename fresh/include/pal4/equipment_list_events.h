#pragma once

#include <cstdint>

#include "pal4/equipment_window.h"

namespace pal4 {

struct EquipmentListMemberState {
    bool equipment_highlighted{};
};

class IEquipmentListEventsRuntime {
public:
    virtual ~IEquipmentListEventsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void RemovePrimaryMember() = 0;
    virtual void InsertSecondaryMember() = 0;
    virtual void SetEquipmentVisible(EquipmentWindowState& state,
                                     bool visible) = 0;
    [[nodiscard]] virtual bool IsDisabled(std::uint32_t window) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t window) = 0;
    virtual void SetHoverFlag(std::uint32_t window, bool hovered) = 0;
    virtual void RequestRedraw(std::uint32_t window) = 0;
    virtual void HandleMouseEvent(std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t HoveredItem(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t HoverRowIndex(
        std::uint32_t list) = 0;
    virtual void SetHoverRowIndex(std::uint32_t list,
                                  std::uint32_t row) = 0;
    virtual void UpHoverItem(std::uint32_t list) = 0;
    virtual void DownHoverItem(std::uint32_t list) = 0;
    virtual void HandleItemClick(EquipmentWindowState& state,
                                 float x, float y) = 0;
};

class EquipmentListEventsRuntimeStub : public IEquipmentListEventsRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    void RemovePrimaryMember() override {}
    void InsertSecondaryMember() override {}
    void SetEquipmentVisible(EquipmentWindowState&, bool) override {}
    bool IsDisabled(std::uint32_t) override { return true; }
    bool IsVisible(std::uint32_t) override { return false; }
    void SetHoverFlag(std::uint32_t, bool) override {}
    void RequestRedraw(std::uint32_t) override {}
    void HandleMouseEvent(std::uint32_t) override {}
    std::uint32_t RowCount(std::uint32_t) override { return 0; }
    std::uint32_t HoveredItem(std::uint32_t) override { return 0; }
    std::uint32_t HoverRowIndex(std::uint32_t) override { return 0; }
    void SetHoverRowIndex(std::uint32_t, std::uint32_t) override {}
    void UpHoverItem(std::uint32_t) override {}
    void DownHoverItem(std::uint32_t) override {}
    void HandleItemClick(EquipmentWindowState&, float, float) override {}
};

class EquipmentListEvents final {
public:
    [[nodiscard]] static bool Handle(EquipmentWindowState& state,
                                     EquipmentListMemberState& member,
                                     std::uint32_t event,
                                     IEquipmentListEventsRuntime& runtime);
    [[nodiscard]] static bool ResetHighlight(
        EquipmentWindowState& state, EquipmentListMemberState& member,
        IEquipmentListEventsRuntime& runtime);
    static void SetHighlight(EquipmentWindowState& state,
                             EquipmentListMemberState& member,
                             bool highlighted,
                             IEquipmentListEventsRuntime& runtime);
};

}  // namespace pal4
