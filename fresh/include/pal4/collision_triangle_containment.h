#pragma once

#include <array>

namespace pal4 {

using CollisionPoint3 = std::array<float, 3u>;

class CollisionTriangleContainment final {
public:
    // IDA 0x5fe6c0.
    [[nodiscard]] static bool ContainsProjectedPoint(
        const CollisionPoint3& point,
        const std::array<CollisionPoint3, 3u>& vertices,
        const CollisionPoint3& normal);
    // IDA 0x5feb10.
    [[nodiscard]] static bool IntersectLineWithTriangle(
        const CollisionPoint3& origin,
        const CollisionPoint3& direction,
        const std::array<CollisionPoint3, 3u>& vertices,
        CollisionPoint3* intersection);
};

}  // namespace pal4
