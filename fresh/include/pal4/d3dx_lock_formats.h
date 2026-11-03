#pragma once

#include <cstdint>

namespace pal4::detail {

inline bool IsDxtFormat(const std::uint32_t format) {
    switch (format) {
    case 0x31545844u:
    case 0x32545844u:
    case 0x33545844u:
    case 0x34545844u:
    case 0x35545844u:
        return true;
    default:
        return false;
    }
}

inline bool IsPackedTwoPixelFormat(const std::uint32_t format) {
    switch (format) {
    case 0x32595559u:
    case 0x42475247u:
    case 0x47424752u:
    case 0x59565955u:
        return true;
    default:
        return false;
    }
}

}  // namespace pal4::detail
