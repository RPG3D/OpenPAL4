#pragma once

#include "pal4/property_window_lifecycle.h"

#include <cstdint>

namespace pal4 {

class IPropertyWindowMagicItemRuntime {
public:
    virtual ~IPropertyWindowMagicItemRuntime() = default;
    [[nodiscard]] virtual bool IsDialogVisible(
        PropertyWindowState& state) = 0;
    virtual void ToggleModel(PropertyWindowState& state, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryEntry(
        std::uint32_t id, std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetProperty(std::uint32_t category,
                             std::uint32_t item) = 0;
    virtual void ShowOrHideDialog(std::uint32_t mode,
                                  std::uint32_t callback,
                                  std::uint32_t item) = 0;
    virtual void PopulateList(PropertyWindowState& state,
                              std::uint32_t category,
                              bool preserve_scroll) = 0;
};

class PropertyWindowMagicItem final {
public:
    [[nodiscard]] static bool Use(PropertyWindowState& state,
                                  IPropertyWindowMagicItemRuntime& runtime);
};

}  // namespace pal4
