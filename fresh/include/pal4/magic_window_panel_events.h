#pragma once

#include "pal4/magic_window_panel.h"
#include "pal4/magic_window_selection.h"

#include <cstdint>

namespace pal4 {

struct MagicWindowPanelEventState {
    std::uint32_t panel_owner{};
    bool highlight_active{};
};

struct MagicWindowPoint {
    float x{};
    float y{};
};

struct MagicWindowSyntheticClick {
    std::uint32_t target{};
    MagicWindowPoint cursor;
    std::uint32_t guard{};
    std::uint32_t click_count{1u};
};

class IMagicWindowPanelEventsRuntime : public IMagicWindowSelectionRuntime {
public:
    virtual void HandleMouseEvent(std::uint32_t window) = 0;
    virtual void RemoveActive(std::uint32_t member) = 0;
    virtual void SetStuntPanelVisible(std::uint32_t panel_owner,
                                      bool visible) = 0;
    virtual void UpdateMagicDetails(std::uint32_t panel_owner,
                                    std::int32_t row) = 0;
    virtual void UpdateStuntDetails(std::uint32_t panel_owner,
                                    std::int32_t row) = 0;
    virtual bool IsVisible(std::uint32_t window) = 0;
    virtual void SetHoverFlag(std::uint32_t window, bool value) = 0;
    virtual void RequestRedraw(std::uint32_t window) = 0;
    virtual MagicWindowPoint AbsolutePosition(std::uint32_t window) = 0;
    virtual MagicWindowPoint WindowSize(std::uint32_t window) = 0;
    virtual void DispatchStuntClick(
        std::uint32_t panel_owner,
        const MagicWindowSyntheticClick& event) = 0;
};

class MagicWindowPanelEventsRuntimeStub : public IMagicWindowPanelEventsRuntime {
public:
    bool IsSelected(std::uint32_t) override { return false; }
    void HandleMouseEvent(std::uint32_t) override {}
    void RemoveActive(std::uint32_t) override {}
    void SetStuntPanelVisible(std::uint32_t, bool) override {}
    void UpdateMagicDetails(std::uint32_t, std::int32_t) override {}
    void UpdateStuntDetails(std::uint32_t, std::int32_t) override {}
    bool IsVisible(std::uint32_t) override { return false; }
    void SetHoverFlag(std::uint32_t, bool) override {}
    void RequestRedraw(std::uint32_t) override {}
    MagicWindowPoint AbsolutePosition(std::uint32_t) override { return {}; }
    MagicWindowPoint WindowSize(std::uint32_t) override { return {}; }
    void DispatchStuntClick(std::uint32_t,
                            const MagicWindowSyntheticClick&) override {}
};

class MagicWindowPanelEvents final {
public:
    static bool HandlePanelClick(std::int32_t event_code,
                                 const MagicWindowPanelState& panel,
                                 IMagicWindowPanelEventsRuntime& runtime);
    static bool HandleEvent(std::uint32_t member,
                            std::int32_t event_code,
                            MagicWindowPanelEventState& state,
                            const MagicWindowPanelState& panel,
                            IMagicWindowPanelEventsRuntime& runtime);
    static bool HandleStuntRowsEvent(
        std::uint32_t member, std::int32_t event_code,
        MagicWindowPanelEventState& state,
        const MagicWindowPanelState& panel,
        IMagicWindowPanelEventsRuntime& runtime);
    static bool ResetHighlight(MagicWindowPanelEventState& state,
                               const MagicWindowPanelState& panel,
                               IMagicWindowPanelEventsRuntime& runtime);
    static void SetHighlight(bool requested,
                             MagicWindowPanelEventState& state,
                             const MagicWindowPanelState& panel,
                             IMagicWindowPanelEventsRuntime& runtime);
};

}  // namespace pal4
