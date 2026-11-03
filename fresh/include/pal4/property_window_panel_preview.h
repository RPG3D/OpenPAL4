#pragma once

#include "pal4/property_window_lifecycle.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PropertyEffectHandle {
    std::uint32_t first{};
    std::uint32_t second{};
};

class IPropertyWindowPanelPreviewRuntime {
public:
    virtual ~IPropertyWindowPanelPreviewRuntime() = default;
    virtual void SetVisible(std::uint32_t window, std::uint32_t visible) = 0;
    virtual void SetSelected(std::uint32_t button, bool selected) = 0;
    [[nodiscard]] virtual std::uint32_t Image(
        std::string_view imageset, std::string_view image) = 0;
    virtual void SetImage(std::uint32_t window, std::uint32_t image) = 0;
    [[nodiscard]] virtual std::uint32_t GameObjectCollection() = 0;
    [[nodiscard]] virtual std::uint32_t FindObject(
        std::uint32_t collection, std::uint32_t id) = 0;
    [[nodiscard]] virtual PropertyEffectHandle CreateEffect(
        std::string_view name, const std::array<float, 3>& position,
        std::uint32_t sentinel, std::uint32_t flag) = 0;
    virtual void ResetTemporaryEffect() = 0;
    virtual void UpdateEffectProperties(
        PropertyEffectHandle handle, const std::array<float, 3>& scale,
        std::uint8_t flag) = 0;
    virtual void MarkEffectForUpdate(PropertyEffectHandle handle) = 0;
    virtual void PrepareScripts() = 0;
    [[nodiscard]] virtual std::uint32_t CreateObject(
        std::uint32_t collection, std::string_view directory,
        std::string_view name, std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetScale(std::uint32_t object,
                          const std::array<float, 3>& value) = 0;
    virtual void SetPosition(std::uint32_t object,
                             const std::array<float, 3>& value) = 0;
    virtual void CleanupScripts() = 0;
    virtual void RemoveObject(std::uint32_t collection,
                              std::uint32_t id) = 0;
    virtual void HandleEffectEvent(PropertyEffectHandle handle,
                                   std::uint32_t event,
                                   std::uint32_t flag) = 0;
};

class PropertyWindowPanelPreview final {
public:
    static void SetItemPanel(PropertyWindowState& state,
                             std::uint32_t visible, std::uint32_t event,
                             IPropertyWindowPanelPreviewRuntime& runtime);
    static void ToggleModel(PropertyWindowState& state,
                            std::uint8_t visible, std::uint32_t event,
                            IPropertyWindowPanelPreviewRuntime& runtime);
};

}  // namespace pal4
