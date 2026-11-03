#pragma once

#include "pal4/magic_window_panel.h"

#include <cstdint>

namespace pal4 {

class IMagicWindowSelectionRuntime {
public:
    virtual ~IMagicWindowSelectionRuntime() = default;
    virtual bool IsSelected(std::uint32_t button) = 0;
};

class MagicWindowSelectionRuntimeStub : public IMagicWindowSelectionRuntime {
public:
    bool IsSelected(std::uint32_t) override { return false; }
};

class MagicWindowSelection final {
public:
    static std::int32_t SelectedMagicRow(const MagicWindowPanelState& panel,
                                         IMagicWindowSelectionRuntime& runtime);
    static std::int32_t SelectedStuntRow(const MagicWindowPanelState& panel,
                                         IMagicWindowSelectionRuntime& runtime);
};

}  // namespace pal4
