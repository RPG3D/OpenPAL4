#pragma once
#include "pal4/game_vector_math.h"
namespace pal4 {
// Reuse the recovered game table and vector algorithms; callers retain the
// original x87 double result until their own float storage boundary.
inline double NativeGameApproximateMagnitude(const GameVector3& vector){
    return GameVectorMath::ApproximateMagnitude(vector);
}
}
