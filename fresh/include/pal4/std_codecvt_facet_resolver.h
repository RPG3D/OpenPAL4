#pragma once

#include "pal4/std_codecvt_char.h"
#include <locale>

namespace pal4 {

class StdCodecvtFacetResolver final {
public:
    // The owning locale must outlive the returned facet. Missing facets use
    // the standard library's std::bad_cast behavior.
    [[nodiscard]] static const StdCodecvtChar& Resolve(const std::locale& locale);
    static const StdCodecvtChar& Resolve(std::locale&&) = delete;
    static const StdCodecvtChar& Resolve(const std::locale&&) = delete;
};

}  // namespace pal4
