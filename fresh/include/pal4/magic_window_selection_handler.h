#pragma once

#include "pal4/magic_window_requirements.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct MagicSelectionButton {
    std::uint32_t record_key{};
    std::uint32_t unknown_04{};
    std::uint32_t command_address{};
    std::string text;
    bool enabled{};
};

class IMagicWindowSelectionDialogRuntime {
public:
    virtual ~IMagicWindowSelectionDialogRuntime() = default;
    virtual void HideAndCleanup() = 0;
    virtual std::string TriggerTextUtf8(std::uint32_t trigger_id) = 0;
    virtual void CreateButton(const MagicSelectionButton& button) = 0;
    virtual void ShowDialog(std::uint32_t event_payload) = 0;
};

class MagicWindowSelectionDialogRuntimeStub :
    public IMagicWindowSelectionDialogRuntime {
public:
    void HideAndCleanup() override {}
    std::string TriggerTextUtf8(std::uint32_t) override { return {}; }
    void CreateButton(const MagicSelectionButton&) override {}
    void ShowDialog(std::uint32_t) override {}
};

class MagicWindowSelectionHandler final {
public:
    static constexpr std::uint32_t kButtonCommandAddress = 0x8a81f0u;

    static bool Handle(std::uint32_t event_payload,
                       const MagicDbStuntRecord* record,
                       const MagicWindowState& state,
                       IMagicWindowRequirementsRuntime& requirements_runtime,
                       IMagicWindowSelectionDialogRuntime& dialog_runtime);
};

}  // namespace pal4
