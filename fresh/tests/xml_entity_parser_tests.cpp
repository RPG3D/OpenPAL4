#include "pal4/xml_entity_parser.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestNamedEntities() {
    struct Case { const char* input; std::uint8_t value; std::size_t length; };
    for (const Case test : {Case{"&amp;tail", '&', 5u},
                            Case{"&lt;", '<', 4u},
                            Case{"&gt;", '>', 4u},
                            Case{"&quot;", '"', 6u}}) {
        std::array<std::uint8_t, 4> output{0xccu, 0xccu, 0xccu, 0xccu};
        std::uint32_t count = 99u;
        const auto text = reinterpret_cast<const std::uint8_t*>(test.input);
        Check(pal4::XmlEntityParser::Parse(text, output.data(), count, 1) ==
                  text + test.length && count == 1u &&
                  output[0] == test.value && output[1] == 0xccu,
              "recognized named entity consumes its exact prefix");
    }
    const auto text = reinterpret_cast<const std::uint8_t*>("&apos;");
    std::array<std::uint8_t, 4> output{0xccu, 0xccu, 0xccu, 0xccu};
    std::uint32_t count = 99u;
    Check(pal4::XmlEntityParser::Parse(text, output.data(), count, 1) ==
              text + 1u && count == 0u && output[0] == '&',
          "IDA table stops before apos entry");
    const auto unknown = reinterpret_cast<const std::uint8_t*>("&nope;");
    Check(pal4::XmlEntityParser::Parse(unknown, output.data(), count, 0) ==
              unknown + 1u && count == 0u && output[0] == '&',
          "unknown entity copies one byte without reporting a decoded byte");
}

void TestNumericEntities() {
    std::array<std::uint8_t, 4> output{0xccu, 0xccu, 0xccu, 0xccu};
    std::uint32_t count = 99u;
    const auto decimal = reinterpret_cast<const std::uint8_t*>("&#169;!");
    Check(pal4::XmlEntityParser::Parse(decimal, output.data(), count, 1) ==
              decimal + 6u && count == 2u && output[0] == 0xc2u &&
              output[1] == 0xa9u && output[2] == 0xccu,
          "decimal code point uses existing UTF-8 encoder");
    const auto hex = reinterpret_cast<const std::uint8_t*>("&#x1F600;");
    Check(pal4::XmlEntityParser::Parse(hex, output.data(), count, 1) ==
              hex + 9u && count == 4u && output[0] == 0xf0u &&
              output[1] == 0x9fu && output[2] == 0x98u &&
              output[3] == 0x80u,
          "mixed-case hexadecimal digits form a four-byte code point");
    const auto narrow = reinterpret_cast<const std::uint8_t*>("&#x1234;");
    Check(pal4::XmlEntityParser::Parse(narrow, output.data(), count, 0) ==
              narrow + 8u && count == 1u && output[0] == 0x34u,
          "non-UTF mode truncates numeric value to one byte");
    const auto empty_hex = reinterpret_cast<const std::uint8_t*>("&#x;");
    Check(pal4::XmlEntityParser::Parse(empty_hex, output.data(), count, 1) ==
              empty_hex + 4u && count == 1u && output[0] == 0u,
          "empty hexadecimal digit sequence decodes zero");
    const auto empty_dec = reinterpret_cast<const std::uint8_t*>("&#;");
    Check(pal4::XmlEntityParser::Parse(empty_dec, output.data(), count, 0) ==
              empty_dec + 3u && count == 1u && output[0] == 0u,
          "empty decimal digit sequence decodes zero");
    output.fill(0xccu);
    const auto beyond = reinterpret_cast<const std::uint8_t*>("&#x200000;");
    Check(pal4::XmlEntityParser::Parse(beyond, output.data(), count, 1) ==
              beyond + 10u && count == 0u && output[0] == 0xccu,
          "out-of-range UTF-8 value still consumes the numeric entity");
}

void TestMalformedNumericEntities() {
    for (const auto input : {"&#x", "&#xG;", "&#12a;", "&#X41;",
                             "&#19", "&#x1 2;"}) {
        std::array<std::uint8_t, 4> output{0xccu, 0xccu, 0xccu, 0xccu};
        std::uint32_t count = 99u;
        Check(pal4::XmlEntityParser::Parse(
                  reinterpret_cast<const std::uint8_t*>(input),
                  output.data(), count, 1) == nullptr && count == 0u &&
                  output[0] == 0xccu,
              "malformed numeric entity fails without changing output");
    }
}

}  // namespace

int main() {
    TestNamedEntities();
    TestNumericEntities();
    TestMalformedNumericEntities();
}
