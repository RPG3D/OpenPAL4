#pragma once

#include "pal4/property_window_lifecycle.h"

#include <cstdint>

namespace pal4 {

class IPropertyWindowCallbacksRuntime {
public:
    virtual ~IPropertyWindowCallbacksRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void InsertActiveMember(PropertyWindowState& state,
                                    std::uint32_t offset) = 0;
    virtual void SetImage(std::uint32_t window,
                          std::uint32_t image) = 0;
    virtual void PopulateList(PropertyWindowState& state,
                              std::uint32_t category,
                              bool preserve_scroll) = 0;
    virtual void HandleSelection(PropertyWindowState& state,
                                 std::uint32_t event_data) = 0;
};

class PropertyWindowCallbacks final {
public:
    [[nodiscard]] static bool OnCategoryClick(
        PropertyWindowState& state, std::uint32_t event,
        IPropertyWindowCallbacksRuntime& runtime);
    [[nodiscard]] static bool UpdatePanel(
        PropertyWindowState& state,
        IPropertyWindowCallbacksRuntime& runtime);
    [[nodiscard]] static bool OnItemClick(
        PropertyWindowState& state, std::uint32_t event,
        IPropertyWindowCallbacksRuntime& runtime);
};

}  // namespace pal4
