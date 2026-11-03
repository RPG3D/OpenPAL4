#pragma once

#include "pal4/magic_window_panel.h"

#include <cstdint>

namespace pal4 {

class IMagicWindowActionButtonRuntime {
public:
    virtual ~IMagicWindowActionButtonRuntime() = default;
    virtual void SetVisible(std::uint32_t button, bool visible) = 0;
    virtual void SetEnabled(std::uint32_t button, bool enabled) = 0;
    virtual void SetKey(std::uint32_t button, std::uint32_t key) = 0;
};

class MagicWindowActionButtonRuntimeStub : public IMagicWindowActionButtonRuntime {
public:
    void SetVisible(std::uint32_t, bool) override {}
    void SetEnabled(std::uint32_t, bool) override {}
    void SetKey(std::uint32_t, std::uint32_t) override {}
};

class MagicWindowActionButtons final {
public:
    static void Reset(const MagicWindowPanelState& panel,
                      IMagicWindowActionButtonRuntime& runtime);
};

}  // namespace pal4
