#pragma once

#include "pal4/grid_vector_statics.h"

namespace pal4 {

class GridVectorMath final {
public:
    [[nodiscard]] static GridVector3& ConstrainDirection(
        GridVector3& destination, bool adjust_below_threshold,
        const GridVector3& velocity, float threshold,
        const GridVector3& normal);
};

}  // namespace pal4
