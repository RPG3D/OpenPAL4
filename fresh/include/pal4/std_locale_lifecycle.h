#pragma once

#include <locale>

// Host standard-library type; not a VC6 binary-layout record.
namespace pal4 {
using StdLocaleLifecycle = std::locale;
}  // namespace pal4
