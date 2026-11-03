#pragma once

#include <string>
#include <string_view>

namespace pal4 {

class UppercaseStringPair final {
public:
    // IDA 0x500b40: copy two string buffers, then uppercase each C string.
    UppercaseStringPair(std::string_view first, std::string_view second);

    [[nodiscard]] const std::string& First() const noexcept { return first_; }
    [[nodiscard]] const std::string& Second() const noexcept { return second_; }

private:
    std::string first_;
    std::string second_;
};

}  // namespace pal4
