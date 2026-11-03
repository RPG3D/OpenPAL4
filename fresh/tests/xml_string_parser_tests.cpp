#include "pal4/xml_string_parser.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestRawParsing() {
    const pal4::XmlStringParser parser{true};
    const std::uint8_t text[]{' ', 'A', '&', 'a', 'm', 'p', ';',
                              'B', '<', '/', 'x', '>', 0u};
    const std::uint8_t delimiter[]{'<', '/', 0u};
    std::string output = "stale";
    Check(parser.Parse(text, output, false, delimiter, false, 0) ==
              text + 10u && output == " A&B",
          "raw branch retains whitespace, decodes entity and consumes delimiter");
    const std::uint8_t plain[]{'x', ' ', 'y', '|', 0u};
    const std::uint8_t bar[]{'|', 0u};
    Check(pal4::XmlStringParser{false}.Parse(
              plain, output, true, bar, false, 0) == plain + 4u &&
              output == "x y", "disabled normalization retains spaces");
    const std::uint8_t malformed[]{'a', '&', '#', 'x', 'Q', ';', 0u};
    Check(parser.Parse(malformed, output, false, bar, false, 1) == nullptr &&
              output == "a", "malformed numeric entity stops after prefix");
}

void TestNormalizedParsing() {
    const pal4::XmlStringParser parser{true};
    const std::uint8_t text[]{' ', '\t', 'A', '\r', '\n', ' ', 'B',
                              '&', 'l', 't', ';', ' ', ' ', 'E', 'N', 'D', 0u};
    const std::uint8_t delimiter[]{'e', 'n', 'd', 0u};
    std::string output = "stale";
    Check(parser.Parse(text, output, true, delimiter, true, 0) ==
              text + 16u && output == "A B<",
          "normalized branch trims leading and trailing whitespace");
    const std::uint8_t marked[]{0xefu, 0xbbu, 0xbfu,
                                'A', '\t', 'B', 0u};
    const std::uint8_t bang[]{'!', 0u};
    Check(parser.Parse(marked, output, true, bang, false, 1) ==
              marked + 7u && output == "A B",
          "normalization skips UTF-8 marker and collapses tab");
    const std::uint8_t all_space[]{' ', '\r', '\n', 0u};
    Check(parser.Parse(all_space, output, true, bang, false, 0) ==
              all_space + 4u && output.empty(),
          "trailing whitespace is dropped at NUL");
    const std::uint8_t empty[]{0u};
    Check(parser.Parse(empty, output, true, bang, false, 0) == nullptr &&
              output.empty(), "initial empty input returns null after clearing");
}

void TestUtf8AndUnknownEntity() {
    const pal4::XmlStringParser parser{true};
    const std::uint8_t text[]{0xe6u, 0xb1u, 0x89u,
                              '&', 'a', 'p', 'o', 's', ';', '|', 0u};
    const std::uint8_t bar[]{'|', 0u};
    std::string output;
    Check(parser.Parse(text, output, false, bar, false, 1) ==
              text + 10u && output.size() == 8u &&
              static_cast<std::uint8_t>(output[0]) == 0xe6u &&
              output.substr(3u) == "apos;",
          "UTF-8 passes through and unmatched entity drops ampersand");
    const std::uint8_t truncated[]{'a', 0xe6u, 0u};
    Check(parser.Parse(truncated, output, false, bar, false, 1) == nullptr &&
              output.size() == 2u && output[0] == 'a' &&
              static_cast<std::uint8_t>(output[1]) == 0xe6u,
          "truncated multibyte text stops before crossing terminator");
}

}  // namespace

int main() {
    TestRawParsing();
    TestNormalizedParsing();
    TestUtf8AndUnknownEntity();
}
