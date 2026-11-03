#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class StringCaseUtilities final {
public:
    // IDA 0x642210: duplicate a C string and convert its bytes to uppercase.
    [[nodiscard]] static std::string UppercaseCopy(std::string_view source);
    // IDA 0x642320: uppercase both C strings, then compare bytes and length.
    [[nodiscard]] static std::int32_t CompareInsensitive(
        const char* first, const char* second);
};

}  // namespace pal4
