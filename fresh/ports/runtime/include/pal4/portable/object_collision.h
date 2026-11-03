#pragma once
#include "pal4/portable/actor.h"
#include "pal4/portable/world.h"
namespace pal4::portable {
struct ObjectCollisionAtomic {std::uint32_t frame{},flags{};std::vector<CollisionTriangle> triangles;};
struct ObjectCollisionGeometry {std::vector<ObjectCollisionAtomic> atomics;};
ObjectCollisionGeometry ReadObjectCollision(std::istream& input);
void ValidateObjectCollision(const ObjectCollisionGeometry& geometry,const Actor& actor);
std::vector<CollisionTriangle> TransformObjectCollision(const ObjectCollisionAtomic& atomic,
    std::span<const Matrix> frames,const Matrix& model);
}
