#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IUiIconImagesRuntime {
public:
    virtual ~IUiIconImagesRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetImageset(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t GetImage(
        std::uint32_t imageset, std::string_view name) = 0;
};

class IUiIconCatalogRuntime : public IUiIconImagesRuntime {
public:
    virtual void InitializePlayerManager() = 0;
    [[nodiscard]] virtual bool IsPlayer102Active() = 0;
};

class UiIconCatalog final {
public:
    [[nodiscard]] static std::uint32_t GetIconImageForPlayerState(
        std::int32_t character, std::int32_t category,
        bool player102_active, IUiIconImagesRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetIconImage(
        std::int32_t character, std::int32_t category,
        std::int32_t player_override, IUiIconCatalogRuntime& runtime);
};

}  // namespace pal4
