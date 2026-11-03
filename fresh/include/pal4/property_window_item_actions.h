#pragma once

#include "pal4/property_window_lifecycle.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPropertyWindowItemActionsRuntime {
public:
    virtual ~IPropertyWindowItemActionsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectAddress(
        const PropertyWindowState& state) = 0;
    [[nodiscard]] virtual std::uint32_t HoveredItem(
        PropertyWindowState& state) = 0;
    virtual void UpdateDetails(PropertyWindowState& state,
                               std::uint32_t item) = 0;
    virtual void SetProperty(std::uint32_t category,
                             std::uint32_t item) = 0;
    virtual void ExecuteEarthballSkip(PropertyWindowState& state) = 0;
    virtual void HandleMagicItem(PropertyWindowState& state) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void ShowOrHideDialog(std::uint32_t mode,
                                  std::uint32_t callback,
                                  std::uint32_t item) = 0;
    virtual void ReleaseCallback(std::uint32_t callback,
                                 std::uint32_t flag) = 0;
    virtual void PlayMusic(std::string_view file, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ItemTargetType(
        std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t index) = 0;
    virtual void ApplyItemEffect(std::uint32_t caster,
                                 std::uint32_t effect,
                                 std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveItem(
        std::uint32_t item_id, std::uint32_t amount) = 0;
    virtual void PopulateList(PropertyWindowState& state,
                              std::uint32_t category,
                              bool preserve_scroll) = 0;
};

class PropertyWindowItemActions final {
public:
    static constexpr std::uint32_t kUseCallback = 0x473000u;
    static constexpr std::uint32_t kCallbackVtable = 0x843a20u;

    [[nodiscard]] static bool HandleHover(
        PropertyWindowState& state,
        IPropertyWindowItemActionsRuntime& runtime);
    [[nodiscard]] static bool ProcessClick(
        PropertyWindowState& state, std::uint32_t event,
        IPropertyWindowItemActionsRuntime& runtime);
    [[nodiscard]] static bool UseItem(
        PropertyWindowState& state, std::uint32_t choice,
        IPropertyWindowItemActionsRuntime& runtime);
};

}  // namespace pal4
