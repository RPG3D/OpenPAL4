#pragma once

#include <array>
#include <cmath>

namespace pal4::movement_detail {
// Game CalculateDotProduct40BF20, not the RenderWare vector API. Preserve
// its Z,Y,X association and the rounding of each original PC24 instruction.
inline float Dot(const std::array<float,3>& a,const std::array<float,3>& b) {
    const volatile float z=a[2]*b[2];
    const volatile float y=a[1]*b[1];
    const volatile float zy=z+y;
    const volatile float x=a[0]*b[0];
    return zy+x;
}
inline float Length(const std::array<float,3>& vector) {
    return static_cast<float>(std::sqrt(static_cast<double>(Dot(vector,vector))));
}
inline std::array<float,3> Normalize(const std::array<float,3>& vector) {
    const auto length=Length(vector);
    if(!(length>0.f))return vector;
    const volatile float inverse=1.f/length;
    return {inverse*vector[0],inverse*vector[1],inverse*vector[2]};
}
} // namespace pal4::movement_detail
