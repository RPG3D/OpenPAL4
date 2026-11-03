#pragma once

#include "pal4/property_window_lifecycle.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IPropertyWindowModelPreviewRuntime {
public:
    virtual ~IPropertyWindowModelPreviewRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GameObjectCollection() = 0;
    [[nodiscard]] virtual std::uint32_t FindObject(
        std::uint32_t collection, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveObject(
        std::uint32_t collection, std::uint32_t id) = 0;
    [[nodiscard]] virtual int CompareNamesIgnoreCase(
        std::uint32_t left, std::uint32_t right) = 0;
    virtual void PrepareScripts() = 0;
    [[nodiscard]] virtual std::uint32_t CleanupScripts() = 0;
    [[nodiscard]] virtual std::uint32_t CreateObject(
        std::uint32_t collection, std::string_view directory,
        std::uint32_t model_name, std::uint32_t flag) = 0;
    virtual void SetTexture(std::uint32_t object,
                            std::uint32_t texture) = 0;
    virtual void SetBaseTransform(
        std::uint32_t object, const std::array<float, 3>& value,
        std::uint32_t flag) = 0;
    virtual void SetPosition(
        std::uint32_t object, const std::array<float, 3>& value) = 0;
    virtual void SetRotation(
        std::uint32_t object, std::uint32_t axis,
        const std::array<float, 3>& value) = 0;
};

class PropertyWindowModelPreview final {
public:
    static constexpr std::uint32_t kEmptyTextAddress = 0x841964u;

    [[nodiscard]] static std::uint32_t Update(
        PropertyWindowState& state, std::uint32_t record,
        IPropertyWindowModelPreviewRuntime& runtime);
};

}  // namespace pal4
