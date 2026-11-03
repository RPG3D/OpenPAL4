#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

class XmlStringParser final {
public:
    explicit XmlStringParser(bool normalization_enabled)
        : normalization_enabled_(normalization_enabled) {}

    // IDA 0x5f21d0. Returns the byte following the delimiter.
    [[nodiscard]] const std::uint8_t* Parse(
        const std::uint8_t* text, std::string& output,
        bool normalize, const std::uint8_t* delimiter,
        bool ignore_case, int utf8_mode) const;

private:
    bool normalization_enabled_;
};

}  // namespace pal4
