#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string_view>

namespace pal4 {

class IMinimapSpriteRuntime {
public:
    virtual ~IMinimapSpriteRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetMinimap() = 0;
    virtual void CleanupSection(std::uint32_t minimap,
                                std::uint32_t section) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t IterateObjects(
        std::uint32_t manager,
        const std::function<std::uint32_t(std::uint32_t)>& callback,
        std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::array<float, 3> ReadPosition(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::array<float, 2> ConvertGridCoordinates(
        std::uint32_t minimap, const std::array<float, 3>& position) = 0;
    [[nodiscard]] virtual std::int32_t ConvertFloatToInt(float value) = 0;
    virtual void AddIcon(std::uint32_t minimap, std::uint32_t object_label,
                         std::uint32_t category, std::string_view image,
                         std::int32_t x, std::int32_t y) = 0;
};

class MinimapSpriteLoader final {
public:
    [[nodiscard]] static std::uint32_t InitializeAndLoad(
        IMinimapSpriteRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeThunk(
        IMinimapSpriteRuntime& runtime);
    [[nodiscard]] static std::uint32_t LoadObjectIcon(
        std::uint32_t object, IMinimapSpriteRuntime& runtime);
};

}  // namespace pal4
