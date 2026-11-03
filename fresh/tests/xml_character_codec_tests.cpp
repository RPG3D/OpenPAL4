#include "pal4/xml_character_codec.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestUtf8Boundaries() {
    struct Case {
        std::uint32_t codepoint;
        std::vector<std::uint8_t> bytes;
    };
    const std::array cases{
        Case{0u, {0u}},
        Case{0x7fu, {0x7fu}},
        Case{0x80u, {0xc2u, 0x80u}},
        Case{0x7ffu, {0xdfu, 0xbfu}},
        Case{0x800u, {0xe0u, 0xa0u, 0x80u}},
        Case{0xd800u, {0xedu, 0xa0u, 0x80u}},
        Case{0xffffu, {0xefu, 0xbfu, 0xbfu}},
        Case{0x10000u, {0xf0u, 0x90u, 0x80u, 0x80u}},
        Case{0x10ffffu, {0xf4u, 0x8fu, 0xbfu, 0xbfu}},
        Case{0x1fffffu, {0xf7u, 0xbfu, 0xbfu, 0xbfu}}};
    for (const auto& item : cases) {
        std::array<std::uint8_t, 6> output;
        output.fill(0xeeu);
        std::uint32_t length = 99u;
        Check(pal4::XmlCharacterCodec::EncodeUtf8(
                  item.codepoint, output.data() + 1u, length) ==
                  output.data() + 2u &&
                  length == item.bytes.size() &&
                  output.front() == 0xeeu &&
                  output[length + 1u] == 0xeeu,
              "UTF-8 encoder reports byte count and lead-byte successor");
        for (std::size_t index = 0; index < item.bytes.size(); ++index)
            Check(output[index + 1u] == item.bytes[index],
                  "UTF-8 encoder writes exact boundary bytes");
    }
    std::array<std::uint8_t, 6> output;
    output.fill(0xeeu);
    std::uint32_t length = 99u;
    Check(pal4::XmlCharacterCodec::EncodeUtf8(
              0x200000u, output.data() + 1u, length) == nullptr &&
              length == 0u && output[1] == 0xeeu,
          "out-of-range input clears count without touching output");
}

void TestClassification() {
    using Codec = pal4::XmlCharacterCodec;
    Check(Codec::IsAlphaOrExtended('A') != 0 &&
              Codec::IsAlphaOrExtended('z') != 0 &&
              Codec::IsAlphaOrExtended('5') == 0 &&
              Codec::IsAlphaOrExtended('_') == 0 &&
              Codec::IsAlphaOrExtended(0x7fu) == 1 &&
              Codec::IsAlphaOrExtended(0xffu) == 1,
          "alpha helper uses CRT for ASCII and accepts DEL or high bytes");
    Check(Codec::IsAlnumOrExtended('A') != 0 &&
              Codec::IsAlnumOrExtended('5') != 0 &&
              Codec::IsAlnumOrExtended('_') == 0 &&
              Codec::IsAlnumOrExtended(0x7eu) == 0 &&
              Codec::IsAlnumOrExtended(0x7fu) == 1 &&
              Codec::IsAlnumOrExtended(0x80u) == 1,
          "alnum helper uses CRT for ASCII and accepts extended bytes");
}

}  // namespace

int main() {
    TestUtf8Boundaries();
    TestClassification();
}
