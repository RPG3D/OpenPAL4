#pragma once

#include <stdexcept>

// Host standard-library type; not a VC6 binary-layout record.
namespace pal4 {
using InvalidArgumentError = std::invalid_argument;
}  // namespace pal4
