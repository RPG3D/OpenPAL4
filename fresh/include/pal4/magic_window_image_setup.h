#pragma once

#include "pal4/magic_window.h"
#include "pal4/magic_window_panel.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMagicWindowImageSetupRuntime {
public:
    virtual ~IMagicWindowImageSetupRuntime() = default;
    virtual void SetText(std::uint32_t window, std::string_view text) = 0;
    virtual void SetImage(std::uint32_t window, std::string_view imageset,
                          std::string_view image) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetStuntPanelVisible(std::uint32_t object, bool visible) = 0;
    virtual std::uint32_t GetPlayer(std::int32_t character_index) = 0;
    virtual void SetMagicPanelVisible(std::uint32_t object, bool visible) = 0;
    virtual void ResetStuntPanel(std::uint32_t object) = 0;
};

class MagicWindowImageSetupRuntimeStub : public IMagicWindowImageSetupRuntime {
public:
    void SetText(std::uint32_t, std::string_view) override {}
    void SetImage(std::uint32_t, std::string_view,
                  std::string_view) override {}
    void SetVisible(std::uint32_t, bool) override {}
    void SetStuntPanelVisible(std::uint32_t, bool) override {}
    std::uint32_t GetPlayer(std::int32_t) override { return 0; }
    void SetMagicPanelVisible(std::uint32_t, bool) override {}
    void ResetStuntPanel(std::uint32_t) override {}
};

class MagicWindowImageSetup final {
public:
    static std::uint32_t Prepare(std::uint32_t object,
                                 const MagicWindowState& state,
                                 const MagicWindowPanelState& panel,
                                 IMagicWindowImageSetupRuntime& runtime);
};

}  // namespace pal4
