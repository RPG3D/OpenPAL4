#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

using CameraVector3 = std::array<float, 3>;
using CameraMatrixWords = std::array<std::uint32_t, 16>;

}  // namespace pal4
