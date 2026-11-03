#include "pal4/xml_name_parser.h"

#include <array>
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

void TestInvalidStartClearsOutput() {
    std::string name = "stale";
    Check(pal4::XmlNameParser::Read(nullptr, name, 1) == nullptr &&
              name.empty(), "null input clears name");
    const std::uint8_t empty[]{0u};
    name = "stale";
    Check(pal4::XmlNameParser::Read(empty, name, 0) == nullptr &&
              name.empty(), "empty input clears name");
    for (const std::uint8_t initial : {'0', '-', '.', ':', ' ', '?'}) {
        const std::array<std::uint8_t, 3> input{initial, 'a', 0u};
        name = "stale";
        Check(pal4::XmlNameParser::Read(input.data(), name, 1) == nullptr &&
                  name.empty(), "invalid first byte clears name");
    }
}

void TestValidNameAndDelimiter() {
    const std::uint8_t text[]{'_', 'A', '9', '-', '.', ':', 'z', '?', 0u};
    std::string name = "old";
    Check(pal4::XmlNameParser::Read(text, name, 0) == text + 7u &&
              name == "_A9-.:z", "name characters and delimiter");
    const std::uint8_t one[]{'x', 0u};
    Check(pal4::XmlNameParser::Read(one, name, 1) == one + 1u &&
              name == "x", "terminator pointer and replacement");
    const std::uint8_t mixed[]{'a', 'b', ' ', 'c', 0u};
    Check(pal4::XmlNameParser::Read(mixed, name, 1) == mixed + 2u &&
              name == "ab", "space terminates the name");
}

void TestExtendedBytesAndMode() {
    const std::uint8_t extended[]{0x7fu, 0x80u, 0xffu, '3', '/', 0u};
    std::string name;
    const auto expected = std::string{"\x7f\x80\xff" "3", 4u};
    Check(pal4::XmlNameParser::Read(extended, name, 0) == extended + 4u &&
              name == expected, "extended byte range accepted");
    Check(pal4::XmlNameParser::Read(extended, name, 2) == extended + 4u &&
              name == expected, "encoding mode does not change classification");
    const std::uint8_t utf8[]{0xe6u, 0xb1u, 0x89u, ':', '1', '<', 0u};
    Check(pal4::XmlNameParser::Read(utf8, name, 1) == utf8 + 5u &&
              name.size() == 5u, "UTF-8 bytes pass as extended name bytes");
}

}  // namespace

int main() {
    TestInvalidStartClearsOutput();
    TestValidNameAndDelimiter();
    TestExtendedBytesAndMode();
}
