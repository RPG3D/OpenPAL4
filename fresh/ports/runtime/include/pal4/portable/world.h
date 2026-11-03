#pragma once
#include <array>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>
#include <span>

namespace pal4::portable {
using Point3=std::array<float,3>;
struct CollisionTriangle {
    std::array<Point3,3> points;
    std::uint32_t atomic{},material{},triangle{};
    // v1 caches did not retain attributes. v2 explicitly represents absence.
    bool attributes_known{};
    std::optional<std::array<std::uint32_t,3>> rgba;
};
struct CollisionMesh { std::vector<CollisionTriangle> floor,wall; };
struct FloorHit { Point3 position; std::uint32_t triangle; };
CollisionMesh ReadCollision(std::istream& stream);
std::optional<FloorHit> FindFloor(const CollisionMesh& mesh,Point3 position,float above,float below);
// Original downward segment probe: choose the first surface from the top.
std::optional<FloorHit> FindFloorFromAbove(std::span<const CollisionTriangle> triangles,Point3 position,float above,float below);
bool SegmentBlocked(const CollisionMesh& mesh,Point3 from,Point3 to);
struct Scene;
// 原人物前视碰撞使用渲染世界几何，不能用wall.dff替代。
bool WorldSegmentBlocked(const Scene& world,Point3 from,Point3 to);
struct LevelDefinition {
    std::string world,actor;
    Point3 position;
    float yaw;
};
LevelDefinition ReadLevel(std::istream& stream);
std::array<float,16> InstanceMatrix(Point3 position,float yaw_degrees);
std::array<float,16> InstanceMatrix(Point3 position,Point3 rotation);
}  // namespace pal4::portable
