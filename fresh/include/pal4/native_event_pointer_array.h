#pragma once

#include "pal4/string_buffer.h"

#include <span>

namespace pal4 {

// Native ABI storage for the existing event and candidate pointer arrays.
void AppendNativeEventPointer(IStringBufferStorageRuntime& memory,
                               std::span<std::uint32_t, 3> image,
                               std::uint32_t position, std::uint32_t event);

}  // namespace pal4
