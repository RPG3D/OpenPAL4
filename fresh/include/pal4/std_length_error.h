#pragma once

#include <stdexcept>

// Host standard-library type; not a VC6 binary-layout record.
namespace pal4 {
using StdLengthError = std::length_error;
}  // namespace pal4
