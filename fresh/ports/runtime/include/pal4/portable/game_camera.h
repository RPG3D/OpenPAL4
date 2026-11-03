#pragma once
#include "pal4/portable/scene.h"
#include <memory>
#include <iosfwd>

namespace pal4::portable {
// 原GameConfigInitializer默认值；后台SDK相机对照见COMBAT_CAMERA.md。
struct GameCameraConfig {float near_clip{5},far_clip{44700},view_scale{0.5f};};
struct GameCameraPose {
    std::array<float,3> eye{},target{},right{},up{},forward{},angles{};
    float requested_distance{},distance{};
    bool collision{};
};
// Reuses the reconstructed PAL4 matrix, frustum and distance update algorithms.
// CPU camera state survives graphics-context loss. No RenderWare/OS pointers.
class GameCameraSystem {
public:
    explicit GameCameraSystem(GameCameraConfig config={});
    ~GameCameraSystem();
    GameCameraSystem(const GameCameraSystem&)=delete;
    GameCameraSystem& operator=(const GameCameraSystem&)=delete;
    void SetCollision(bool enabled);
    void SetParameters(std::size_t camera,std::array<float,3> target,std::array<float,3> yaw_pitch_roll,float distance);
    void SetTarget(std::size_t camera,std::array<float,3> target);
    // 原任务只读数值快照及参数边界；不公开可写对象或宿主地址。
    std::uint32_t TaskWord(std::size_t camera,std::uint32_t offset) const;
    void TrackVector(std::size_t camera,bool eye,std::array<float,3> value,std::uint32_t mode);
    void TrackScalar(std::size_t camera,unsigned parameter,float value,std::uint32_t mode);
    bool SetMode(unsigned index);
    unsigned Mode() const;
    void SetYaw(float yaw);
    bool TracksBound() const;
    void Update(float elapsed,const Scene& world);
    GameCameraPose Pose(std::size_t camera=0) const;
    std::array<float,16> Matrix(float aspect,std::size_t camera=0) const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
void WriteGameCameraReport(std::ostream& output,const GameCameraSystem& cameras);
} // namespace pal4::portable
