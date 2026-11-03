#pragma once

#include "pal4/magic_window.h"
#include "pal4/magic_window_panel.h"

#include <cstdint>

namespace pal4 {

class IMagicWindowCategoryEventsRuntime {
public:
    virtual ~IMagicWindowCategoryEventsRuntime() = default;
    virtual std::uint32_t ButtonKey(std::uint32_t button) = 0;
    virtual void RemoveActive(std::uint32_t member) = 0;
    virtual void InsertActive(std::uint32_t member) = 0;
    virtual void SetSelected(std::uint32_t button, bool selected) = 0;
    virtual void UpdateCharacterDisplay(std::uint32_t object,
                                        std::int32_t index) = 0;
    virtual void UpdateMagicDetails(std::uint32_t object,
                                    std::int32_t index) = 0;
    virtual void UpdateStuntDetails(std::uint32_t object,
                                    std::int32_t index) = 0;
    virtual std::uint32_t FindStuntRecord(std::uint32_t key) = 0;
    virtual void HandleMagicSelection(std::uint32_t object,
                                      std::uint32_t event_payload,
                                      std::uint32_t record) = 0;
};

class MagicWindowCategoryEventsRuntimeStub :
    public IMagicWindowCategoryEventsRuntime {
public:
    std::uint32_t ButtonKey(std::uint32_t) override { return 0; }
    void RemoveActive(std::uint32_t) override {}
    void InsertActive(std::uint32_t) override {}
    void SetSelected(std::uint32_t, bool) override {}
    void UpdateCharacterDisplay(std::uint32_t, std::int32_t) override {}
    void UpdateMagicDetails(std::uint32_t, std::int32_t) override {}
    void UpdateStuntDetails(std::uint32_t, std::int32_t) override {}
    std::uint32_t FindStuntRecord(std::uint32_t) override { return 0; }
    void HandleMagicSelection(std::uint32_t, std::uint32_t,
                              std::uint32_t) override {}
};

class MagicWindowCategoryEvents final {
public:
    static bool HandleStuntClick(std::uint32_t object,
                                 std::uint32_t clicked_button,
                                 MagicWindowState& state,
                                 const MagicWindowPanelState& panel,
                                 IMagicWindowCategoryEventsRuntime& runtime);
    static bool SelectStunt(std::uint32_t object,
                             const MagicWindowState& state,
                             const MagicWindowPanelState& panel,
                             IMagicWindowCategoryEventsRuntime& runtime);
    static bool HandleMagicHover(std::uint32_t object,
                                 std::uint32_t hovered_button,
                                 IMagicWindowCategoryEventsRuntime& runtime);
    static bool HandleStuntHover(std::uint32_t object,
                                 std::uint32_t hovered_button,
                                 const MagicWindowPanelState& panel,
                                 IMagicWindowCategoryEventsRuntime& runtime);
    static bool HandleMagicButtonClick(
        std::uint32_t object, std::uint32_t event_address,
        std::uint32_t clicked_button, std::uint32_t event_guard,
        IMagicWindowCategoryEventsRuntime& runtime);
};

}  // namespace pal4
