#pragma once

#include <stdexcept>

// Host standard-library type; not a VC6 binary-layout record.
namespace pal4 {
using StdRuntimeError = std::runtime_error;
}  // namespace pal4
