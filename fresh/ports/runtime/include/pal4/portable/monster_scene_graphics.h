#pragma once
#include "pal4/portable/atomic_distance.h"
#include "pal4/portable/gles_renderer.h"
#include "pal4/portable/scene_monster_instances.h"
#include <memory>

namespace pal4::portable {
// Borrowed instance and distance owners must outlive this object. The scene's
// original render dispatcher decides which handles to draw; no activity or
// animation policy belongs here. Draw never advances a playback clock.
class MonsterSceneGraphics final {
public:
    MonsterSceneGraphics(SceneMonsterInstances&,DistanceOptimization&);
    ~MonsterSceneGraphics();
    MonsterSceneGraphics(const MonsterSceneGraphics&)=delete;
    MonsterSceneGraphics& operator=(const MonsterSceneGraphics&)=delete;
    void Upload();
    std::uint32_t Draw(SceneMonsterHandle,const Matrix& view_projection,std::array<float,3> camera);
    // Release with the owning GL context current, or Abandon after context loss.
    // Both retain CPU instances and distance/material state for a later Upload.
    void Release();
    void Abandon();
    bool Uploaded() const noexcept;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
