#pragma once
#include "pal4/portable/scene.h"
#include <functional>
#include <optional>
#include <span>
#include <string_view>
namespace pal4::portable {
Scene ReadBspScene(std::span<const std::uint8_t> bsp,std::span<const std::uint8_t> light_config,
                   const std::function<Texture(std::string_view)>& texture,
                   const std::function<std::optional<Texture>(std::string_view)>& lightmap = {});
}
