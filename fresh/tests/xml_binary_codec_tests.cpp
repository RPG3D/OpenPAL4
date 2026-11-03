#include "pal4/xml_binary_codec.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string_view>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IXmlBinaryCodecRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::uint32_t word_reads = 0u;
    std::uint32_t byte_writes = 0u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        ++word_reads;
        return words[address];
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        ++byte_writes;
        bytes[address] = value;
    }
    void Source(const std::string_view text) {
        for (std::uint32_t index = 0u; index < text.size(); ++index)
            bytes[0x1000u + index] =
                static_cast<std::uint8_t>(text[index]);
        bytes[0x1000u + static_cast<std::uint32_t>(text.size())] = 0u;
    }
};

void TestExactCapacityAndPadding() {
    Runtime exact;
    exact.Source("1a2b");
    exact.words[0x3000u] = 2u;
    exact.bytes[0x2000u] = 0xffu;
    exact.bytes[0x2001u] = 0xffu;
    Check(pal4::XmlBinaryCodec::DecodeHex(
              0x1000u, 0x2000u, 0x3000u, exact) == 2 &&
              exact.bytes[0x2000u] == 0x1au &&
              exact.bytes[0x2001u] == 0x2bu &&
              exact.byte_writes == 4u,
          "exact capacity is cleared then filled from hex pairs");

    Runtime padded;
    padded.Source("1a2b");
    padded.words[0x3000u] = 4u;
    Check(pal4::XmlBinaryCodec::DecodeHex(
              0x1000u, 0x2000u, 0x3000u, padded) == 4 &&
              padded.bytes[0x2000u] == 0x1au &&
              padded.bytes[0x2001u] == 0x2bu &&
              padded.bytes[0x2002u] == 0u &&
              padded.bytes[0x2003u] == 0u,
          "native stop probe can emit zero bytes beyond complete hex pairs");
}

void TestOddAndInvalidDigits() {
    Runtime odd;
    odd.Source("f");
    odd.words[0x3000u] = 3u;
    Check(pal4::XmlBinaryCodec::DecodeHex(
              0x1000u, 0x2000u, 0x3000u, odd) == 1 &&
              odd.bytes[0x2000u] == 0xf0u &&
              odd.bytes[0x2001u] == 0u &&
              odd.bytes[0x2002u] == 0u,
          "odd last nibble becomes the high half of the output byte");

    Runtime invalid;
    invalid.Source("Gz");
    invalid.words[0x3000u] = 3u;
    Check(pal4::XmlBinaryCodec::DecodeHex(
              0x1000u, 0x2000u, 0x3000u, invalid) == 2 &&
              invalid.bytes[0x2000u] == 0u &&
              invalid.bytes[0x2001u] == 0u,
          "uppercase and invalid digits decode as zero nibbles");
}

void TestNullEmptyAndNonpositiveCapacity() {
    Runtime missing;
    missing.words[0x3000u] = 2u;
    missing.bytes[0x2000u] = 0x7au;
    Check(pal4::XmlBinaryCodec::DecodeHex(
              0u, 0x2000u, 0x3000u, missing) == 0 &&
              missing.bytes[0x2000u] == 0x7au &&
              missing.word_reads == 1u && missing.byte_writes == 0u,
          "null source returns after capacity read without clearing output");

    Runtime empty;
    empty.Source("");
    empty.words[0x3000u] = 2u;
    empty.bytes[0x2000u] = 0x7au;
    Check(pal4::XmlBinaryCodec::DecodeHex(
              0x1000u, 0x2000u, 0x3000u, empty) == 0 &&
              empty.bytes[0x2000u] == 0u &&
              empty.byte_writes == 2u,
          "empty non-null source clears positive output capacity");

    for (const std::uint32_t capacity : {0u, 0xffffffffu}) {
        Runtime nonpositive;
        nonpositive.Source("ab");
        nonpositive.words[0x3000u] = capacity;
        nonpositive.bytes[0x2000u] = 0x7au;
        Check(pal4::XmlBinaryCodec::DecodeHex(
                  0x1000u, 0x2000u, 0x3000u, nonpositive) == 0 &&
                  nonpositive.bytes[0x2000u] == 0x7au &&
                  nonpositive.byte_writes == 0u,
              "zero or negative capacity leaves output untouched");
    }
}

void TestOverlappingSourceAndDestination() {
    Runtime runtime;
    runtime.Source("ab");
    runtime.words[0x3000u] = 2u;
    Check(pal4::XmlBinaryCodec::DecodeHex(
              0x1000u, 0x1000u, 0x3000u, runtime) == 0 &&
              runtime.bytes[0x1000u] == 0u &&
              runtime.bytes[0x1001u] == 0u,
          "overlapping output is zeroed before input is inspected");
}

}  // namespace

int main() {
    TestExactCapacityAndPadding();
    TestOddAndInvalidDigits();
    TestNullEmptyAndNonpositiveCapacity();
    TestOverlappingSourceAndDestination();
}
