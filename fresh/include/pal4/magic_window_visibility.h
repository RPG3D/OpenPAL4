#pragma once

#include "pal4/magic_window.h"
#include "pal4/magic_window_panel.h"

#include <cstdint>

namespace pal4 {

class IMagicWindowVisibilityRuntime {
public:
    virtual ~IMagicWindowVisibilityRuntime() = default;
    virtual bool HasGameDataEntry(std::uint32_t id) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void SetParent(std::uint32_t object, std::uint8_t showing,
                           std::uint32_t parent) = 0;
    virtual void SetSelected(std::uint32_t button, bool selected) = 0;
    virtual void SetStuntPanelVisible(std::uint32_t object, bool visible) = 0;
    virtual std::int32_t NextCharacterIndex(std::int32_t current) = 0;
    virtual bool HasActivePlayer(std::int32_t index) = 0;
    virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void ProcessEvent(std::uint32_t event_type,
                              std::uint32_t first,
                              std::uint32_t second) = 0;
    virtual void InsertActive(std::uint32_t member) = 0;
    virtual void RemoveActive(std::uint32_t member) = 0;
    virtual void SelectStunt(std::uint32_t object) = 0;
    virtual void UpdateSystemDialogVisibility(std::uint32_t dialog) = 0;
};

class MagicWindowVisibilityRuntimeStub : public IMagicWindowVisibilityRuntime {
public:
    bool HasGameDataEntry(std::uint32_t) override { return false; }
    void SetEnabled(std::uint32_t, bool) override {}
    void SetParent(std::uint32_t, std::uint8_t, std::uint32_t) override {}
    void SetSelected(std::uint32_t, bool) override {}
    void SetStuntPanelVisible(std::uint32_t, bool) override {}
    std::int32_t NextCharacterIndex(std::int32_t) override { return 4; }
    bool HasActivePlayer(std::int32_t) override { return false; }
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    void ProcessEvent(std::uint32_t, std::uint32_t, std::uint32_t) override {}
    void InsertActive(std::uint32_t) override {}
    void RemoveActive(std::uint32_t) override {}
    void SelectStunt(std::uint32_t) override {}
    void UpdateSystemDialogVisibility(std::uint32_t) override {}
};

class MagicWindowVisibility final {
public:
    static constexpr std::uint32_t kVisibilityEvent = 0x8b1394u;
    static constexpr std::uint32_t kOwnerEvent = 0x8b13a0u;
    static constexpr std::uint32_t kHelpEvent = 0x8b13a4u;

    static void Set(std::uint32_t object, std::uint8_t showing,
                    std::uint32_t parent, MagicWindowState& state,
                    const MagicWindowPanelState& panel,
                    IMagicWindowVisibilityRuntime& runtime);
};

}  // namespace pal4
