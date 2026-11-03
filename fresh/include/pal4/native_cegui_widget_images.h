#pragma once

#include "pal4/minimap_scene_ui_movement_display.h"
#include "pal4/minimap_scene_ui_stats.h"
#include <windows.h>

namespace pal4 {

// Borrowed original CEGUI/OIRAMLOOK exports; no private image-map reconstruction.
class NativeCeguiWidgetImages final {
public:
    explicit NativeCeguiWidgetImages(HMODULE module) : module_(module) {}
    [[nodiscard]] std::uint32_t Imageset(std::string_view name) const;
    [[nodiscard]] std::uint32_t Image(std::uint32_t imageset, std::string_view name) const;
    [[nodiscard]] std::uint32_t Image(std::string_view imageset, std::string_view name) const;
    void SetButtonImage(std::uint32_t button, std::string_view part,
                        MinimapButtonImageState state, std::uint32_t image) const;
    [[nodiscard]] std::uint32_t ButtonImage(std::uint32_t button, std::string_view part,
                                          MinimapButtonImageState state) const;
    void ApplyButtonColor(std::uint32_t button, MinimapFlashColor color) const;
    void RequestRedraw(std::uint32_t window) const;
private:
    HMODULE module_;
};

}  // namespace pal4
