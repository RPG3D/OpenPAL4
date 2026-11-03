#pragma once
#include "pal4/portable/collision_surface.h"
#include <functional>
#include <array>
#include <cstdint>
#include <optional>
namespace pal4::portable {
class ObjectMemory;
using SurfaceMotionImage=std::array<std::uint32_t,40>;
using MovementCollisionQuery=std::function<bool(Point3,Point3)>;
// 借用当前地面与原世界碰撞查询。只负责原CollisionActor状态，不持有GPU。
class SurfaceMotion {
public:
    SurfaceMotion(CollisionSurface& surface,Point3 position,float height,MovementCollisionQuery collision,std::uint32_t actor_token=0x8000);
    // 共享存储必须比运动对象活得更久；Close只释放本对象的路径。
    // constructed提供已构造成员的快照时借用其路径，不再次构造/销毁。
    // 调用者须独占成员、在每次操作（含异常）后回写Image，Close后交回原所有者销毁。
    // 仅接受尚未放置且路径为空的成员；原所有者和shared_memory必须存活更久。
    SurfaceMotion(CollisionSurface&,Point3,float,MovementCollisionQuery,std::uint32_t actor_token,ObjectMemory* shared_memory,
        std::optional<SurfaceMotionImage> constructed=std::nullopt);
    ~SurfaceMotion();
    SurfaceMotion(const SurfaceMotion&)=delete;
    SurfaceMotion& operator=(const SurfaceMotion&)=delete;
    bool Start(Point3 from,Point3 to,std::uint32_t target_face);
    void SetSpeed(float speed);
    void Advance(float elapsed);
    std::uint32_t Advance(float elapsed,std::uint32_t event);
    // Borrowed numeric path tokens belong to this owner; not a save format.
    SurfaceMotionImage Image() const;
    void Stop();
    void Close();
    bool Active() const;
    Point3 Position() const;
    Point3 Velocity() const;
    std::uint32_t Face() const;
    std::uint32_t Status() const;
    std::vector<SurfacePathPoint> Path() const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
