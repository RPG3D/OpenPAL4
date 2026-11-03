#pragma once
#include "pal4/portable/world.h"
#include <functional>
#include <memory>
namespace pal4::portable {
enum class CollisionGroup {intri,wall,object};
struct CollisionObjectProperties {
    std::uint32_t mode{1},owner{},callback{};
    bool hard_block{},proximity{},plugin{},patch{},hierarchy{};
    bool operator==(const CollisionObjectProperties&) const=default;
};
// 拥有世界坐标三角形；原链表和回调只读数值句柄，不保存平台地址。
class SceneCollision {
public:
    using Event=std::function<void(std::uint32_t callback,bool entering,std::uint32_t previous)>;
    explicit SceneCollision(Event event={});
    ~SceneCollision();
    SceneCollision(const SceneCollision&)=delete;
    SceneCollision& operator=(const SceneCollision&)=delete;
    std::uint32_t Add(CollisionGroup group,std::span<const CollisionTriangle> triangles,CollisionObjectProperties properties={},std::function<bool()> eligibility={});
    void Replace(std::uint32_t object,std::span<const CollisionTriangle> triangles);
    void Remove(std::uint32_t object);
    void Resolve(std::array<float,4>& upper,std::array<float,4>& lower,Point3& displacement,std::uint32_t excluded_owner=0);
    void InterruptEventTraversal();
    void WriteReport(std::ostream& output) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
