#pragma once

#include <typeinfo>

// Host standard-library type; not a VC6 binary-layout record.
namespace pal4 {
using StdBadCast = std::bad_cast;
}  // namespace pal4
