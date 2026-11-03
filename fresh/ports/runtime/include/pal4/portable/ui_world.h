#pragma once
#include "pal4/portable/scene.h"
#include "pal4/portable/uv_animation.h"
#include <functional>
#include <optional>
#include <string_view>

namespace pal4::portable {
using UiWorldMemberReader = std::function<std::optional<std::vector<std::uint8_t>>(std::string_view)>;
struct UiWorld {
    Scene world;
    std::optional<Scene> transparent;
    std::shared_ptr<UvAnimationDictionary> world_animations,transparent_animations;
};
// 成员和贴图在内存中解码；返回对象拥有转换后的数据。
UiWorld ReadUiWorld(std::string_view name, const UiWorldMemberReader& read,bool with_animations=false);
}
