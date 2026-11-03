#pragma once

#include <cstdint>

namespace pal4 {

class XmlStringComparator final {
public:
    // IDA 0x5f2120. Checks whether prefix begins source.
    [[nodiscard]] static bool HasPrefix(const std::uint8_t* source,
                                        const std::uint8_t* prefix,
                                        bool ignore_case, int encoding_mode);
};

}  // namespace pal4
