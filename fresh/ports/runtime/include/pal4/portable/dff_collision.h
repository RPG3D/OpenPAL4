#pragma once
#include "pal4/portable/world.h"
#include "pal4/portable/object_collision.h"
#include "pal4/portable/scene.h"
#include <functional>
#include <string_view>
#include <span>
namespace pal4::portable {
// Reads static collision atomics from an original RenderWare clump in memory.
std::vector<CollisionTriangle> ReadDffCollision(std::span<const std::uint8_t> data);
ObjectCollisionGeometry ReadDffEventCollision(std::span<const std::uint8_t> data);
ObjectCollisionGeometry ReadDffObjectCollision(std::span<const std::uint8_t> data);
Scene ReadDffEnvironment(std::span<const std::uint8_t> data,
                         const std::function<Texture(std::string_view)>& texture,
                         bool preserve_atomic_draws=false);
}
