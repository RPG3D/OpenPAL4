#pragma once

#include "pal4/property_window_lifecycle.h"

#include <cstdint>

namespace pal4 {

class IPropertyWindowVisibilityRuntime {
public:
    virtual ~IPropertyWindowVisibilityRuntime() = default;
    virtual void SetParent(PropertyWindowState& state,
                           std::uint8_t visible,
                           std::uint32_t parent) = 0;
    virtual void SetItemPanel(PropertyWindowState& state,
                              bool visible) = 0;
    virtual void Broadcast(std::uint32_t event,
                           std::uint32_t first,
                           std::uint32_t second) = 0;
    virtual void SetSelected(std::uint32_t button,
                             bool selected) = 0;
    virtual void AddActiveMember(PropertyWindowState& state,
                                 std::uint32_t offset) = 0;
    virtual void RemoveActiveMember(PropertyWindowState& state,
                                    std::uint32_t offset) = 0;
    virtual void ToggleModel(PropertyWindowState& state,
                             bool visible) = 0;
    virtual void UpdatePropertyPanel(PropertyWindowState& state) = 0;
    virtual void UpdateSystemDialog(std::uint32_t key) = 0;
};

class PropertyWindowVisibility final {
public:
    static void Set(PropertyWindowState& state, std::uint8_t visible,
                    std::uint32_t parent,
                    IPropertyWindowVisibilityRuntime& runtime);
};

}  // namespace pal4
