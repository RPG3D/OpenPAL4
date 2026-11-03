#pragma once

#include "pal4/magic_window_panel.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMagicWindowPanelVisibilityRuntime {
public:
    virtual ~IMagicWindowPanelVisibilityRuntime() = default;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetImage(std::uint32_t window, std::string_view imageset,
                          std::string_view image) = 0;
    virtual void UpdateMagicDetails(std::uint32_t object,
                                     std::int32_t index) = 0;
};

class MagicWindowPanelVisibilityRuntimeStub : public IMagicWindowPanelVisibilityRuntime {
public:
    void SetVisible(std::uint32_t, bool) override {}
    void SetImage(std::uint32_t, std::string_view,
                  std::string_view) override {}
    void UpdateMagicDetails(std::uint32_t, std::int32_t) override {}
};

class MagicWindowPanelVisibility final {
public:
    static void Set(std::uint32_t object, bool visible,
                    const MagicWindowPanelState& panel,
                    IMagicWindowPanelVisibilityRuntime& runtime);
    static void SetStunt(bool visible, const MagicWindowPanelState& panel,
                         IMagicWindowPanelVisibilityRuntime& runtime);
};

}  // namespace pal4
