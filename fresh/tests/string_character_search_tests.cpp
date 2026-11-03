#include "pal4/string_character_search.h"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    using pal4::StringCharacterSearch;
    Check(StringCharacterSearch::FindFirstOf("abcde", 0, "xyc") == 2,
          "first set member found");
    Check(StringCharacterSearch::FindFirstOf("abcde", 3, "acde") == 3,
          "start offset limits search");
    Check(StringCharacterSearch::FindFirstOf("abcde", 4, "e") == 4,
          "last character included");
    Check(StringCharacterSearch::FindFirstOf("abcde", 0, "xyz") == -1,
          "no match returns minus one");
    Check(StringCharacterSearch::FindFirstOf("abc", 0, "") == -1 &&
              StringCharacterSearch::FindFirstOf("abc", 3, "a") == -1 &&
              StringCharacterSearch::FindFirstOf("abc", -1, "a") == -1,
          "empty set and unsigned start bounds");
    constexpr std::string_view high_byte{"a\x80z", 3};
    constexpr std::string_view set{"\x80", 1};
    Check(StringCharacterSearch::FindFirstOf(high_byte, 0, set) == 1,
          "memchr matches high-bit bytes");
    constexpr std::string_view embedded_nul{"a\0b", 3};
    constexpr std::string_view nul_set{"\0", 1};
    Check(StringCharacterSearch::FindFirstOf(embedded_nul, 0, nul_set) == 1,
          "length controls embedded zero search");
}
