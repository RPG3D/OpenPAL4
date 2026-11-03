#include "camera_track_geometry.h"
#include "pal4/game_vector_math.h"
#define HANDMADE_MATH_NO_SSE
#include <HandmadeMath.h>
#include <stdexcept>

namespace pal4::portable::detail {
CameraTrackGeometry::CameraTrackGeometry(){(void)math_.CalculateSquareRootTable();}
void CameraTrackGeometry::ClampDirection(pal4::CameraTaskVector& direction) {
    pal4::GameVector3 value{direction.x,direction.y,direction.z};
    if(value[0]==0 && value[1]==0 && value[2]==0)throw std::runtime_error("zero camera track direction");
    pal4::GameVectorMath::NormalizeAndClamp(value,math_);direction={value[0],value[1],value[2]};
}
void CameraTrackGeometry::Normalize(pal4::CameraTaskVector& value) {
    if(value.x==0 && value.y==0 && value.z==0)throw std::runtime_error("degenerate camera track basis");
    const auto result=HMM_NormV3(HMM_V3(value.x,value.y,value.z));value={result.X,result.Y,result.Z};
}
pal4::CameraTaskQuaternion CameraTrackGeometry::ToQuaternion(const pal4::CameraTaskBasis& basis) {
    auto matrix=HMM_M4D(1);
    matrix.Columns[0]=HMM_V4(basis.right.x,basis.right.y,basis.right.z,0);
    matrix.Columns[1]=HMM_V4(basis.up.x,basis.up.y,basis.up.z,0);
    matrix.Columns[2]=HMM_V4(basis.forward.x,basis.forward.y,basis.forward.z,0);
    const auto q=HMM_M4ToQ_RH(matrix);return {q.X,q.Y,q.Z,q.W};
}
pal4::CameraTaskVector CameraTrackGeometry::ToEuler(const pal4::CameraTaskQuaternion& quaternion) {
    pal4::CameraTaskVector result;(void)pal4::CameraTaskOrientation::ToEuler(quaternion,result);return result;
}
}
