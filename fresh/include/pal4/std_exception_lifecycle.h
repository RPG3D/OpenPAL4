#pragma once

#include <exception>

// Host standard-library type; not a VC6 binary-layout record.
namespace pal4 {
using StdExceptionLifecycle = std::exception;
}  // namespace pal4
