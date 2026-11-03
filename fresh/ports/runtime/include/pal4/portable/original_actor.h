#pragma once
#include "pal4/portable/actor.h"
#include <functional>
#include <span>
#include <string_view>
namespace pal4::portable {
// The callback provides decoded textures from the user's original resource package.
Actor ReadOriginalActor(std::span<const std::uint8_t> dff,
                        const std::function<Texture(std::string_view)>& texture,
                        std::string_view diffuse_override={},
                        const std::function<Texture()>& override_texture={});
}
