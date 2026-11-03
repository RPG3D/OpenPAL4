#include "pal4/uppercase_string_pair.h"

#include <cctype>

namespace pal4 {
namespace {

std::string CopyCString(std::string_view source) {
    const auto end = source.find('\0');
    return std::string(source.substr(0, end));
}

void Uppercase(std::string& text) {
    for (auto& ch : text) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
}

}  // namespace

UppercaseStringPair::UppercaseStringPair(std::string_view first,
                                         std::string_view second)
    : first_(CopyCString(first)), second_(CopyCString(second)) {
    Uppercase(first_);
    Uppercase(second_);
}

}  // namespace pal4
