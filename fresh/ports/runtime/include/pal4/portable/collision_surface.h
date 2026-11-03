#pragma once
#include "pal4/portable/world.h"
#include <memory>
namespace pal4::portable {
// P4CL原地面三角形的选面/投影；输入保留DFF流顺序，恢复SDK顶点顺序后建面。
struct SurfacePathPoint {Point3 position{};std::uint32_t face{};};
// 句柄是本集合索引，不是宿主指针。导航复用原邻接、队列和跨面处理器。
class CollisionSurface {
public:
    explicit CollisionSurface(std::span<const CollisionTriangle> floor);
    ~CollisionSurface();
    CollisionSurface(const CollisionSurface&)=delete;
    CollisionSurface& operator=(const CollisionSurface&)=delete;
    std::size_t Size() const;
    std::uint32_t Select(Point3 point);
    Point3 Project(std::uint32_t face,Point3 point);
    std::uint32_t Snap(Point3& point);
    void EnableNavigation();
    bool TracePath(std::uint32_t from_face,Point3 from,std::uint32_t target_face,Point3 target,std::vector<SurfacePathPoint>& output);
    std::uint32_t Trace(std::uint32_t face,Point3 from,Point3 to,std::uint32_t& neighbor);
    std::uint32_t Advance(Point3 from,std::uint32_t face,Point3& to,std::uint32_t& final_face);
    void Clamp(std::uint32_t face,Point3& point);
    std::size_t LinkedEdges() const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
