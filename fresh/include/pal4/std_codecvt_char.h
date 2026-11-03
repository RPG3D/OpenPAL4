#pragma once

#include <locale>
#include <cwchar>

// Host standard-library type; not a VC6 binary-layout record.
namespace pal4 {
using StdCodecvtChar = std::codecvt<char, char, std::mbstate_t>;
}  // namespace pal4
