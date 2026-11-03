#pragma once
#include "pal4/camera_task_object.h"
#include "pal4/game_math_tables.h"
namespace pal4::portable::detail {
// 原游戏数学与公开数学库之间的适配；供独立SDK对照工具使用同一实现。
class CameraTrackGeometry final:public pal4::ICameraTaskGeometry {
public:
    CameraTrackGeometry();
    void ClampDirection(pal4::CameraTaskVector& direction) override;
    void Normalize(pal4::CameraTaskVector& vector) override;
    pal4::CameraTaskQuaternion ToQuaternion(const pal4::CameraTaskBasis& basis) override;
    pal4::CameraTaskVector ToEuler(const pal4::CameraTaskQuaternion& quaternion) override;
private:pal4::GameMathTables math_;
};
}
