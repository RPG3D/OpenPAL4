#include "pal4/cegui_text_converter.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::ICeguiTextConversionRuntime {
public:
    std::int32_t wide_count = 3;
    std::int32_t byte_count = 5;
    std::int32_t expected_source_length = 3;
    std::uint32_t grown_wide = 0x3000u;
    std::uint32_t grown_byte = 0x4000u;
    std::vector<std::string> events;
    std::uint32_t wide_terminator = 0xffffffffu;
    std::uint32_t byte_terminator = 0xffffffffu;

    std::int32_t MultiByteToWideChar(
        std::uint32_t code_page, std::uint32_t flags,
        std::string_view source, std::int32_t length,
        std::uint32_t destination, std::int32_t capacity) override {
        Check(code_page == 0u && flags == 0u &&
                  length == expected_source_length &&
                  source.size() >= static_cast<std::size_t>(length),
              "ACP conversion receives strlen excluding terminator");
        if (destination == 0u && capacity == 0) {
            events.emplace_back("wide-query");
        } else {
            Check(capacity == wide_count,
                  "second ACP call uses queried character count");
            events.emplace_back("wide-write");
        }
        return wide_count;
    }
    std::int32_t WideCharToMultiByte(
        std::uint32_t code_page, std::uint32_t flags,
        std::uint32_t source, std::int32_t source_length,
        std::uint32_t destination, std::int32_t capacity) override {
        Check(code_page == 0xfde9u && flags == 0u &&
                  source_length == wide_count &&
                  source == (grown_wide == 0u ? 0u : current_wide),
              "UTF-8 conversion uses prepared wide buffer and code page");
        if (destination == 0u && capacity == 0) {
            events.emplace_back("byte-query");
        } else {
            Check(capacity == byte_count,
                  "second UTF-8 call uses queried byte count");
            events.emplace_back("byte-write");
        }
        return byte_count;
    }
    void InsertWide(pal4::StaticCharacterBufferState& state,
                    std::uint32_t at, std::uint32_t count,
                    std::uint16_t fill) override {
        Check(at == state.wide_end && fill == 0u,
              "wide growth inserts zeros at former end");
        events.emplace_back("wide-grow:" + std::to_string(count));
        const auto old_capacity = state.wide_begin == 0u
            ? 0u : (state.wide_end - state.wide_begin) / 2u;
        state.wide_begin = grown_wide;
        state.wide_end = grown_wide + (old_capacity + count) * 2u;
        current_wide = grown_wide;
    }
    void CopyWide(pal4::StaticCharacterBufferState&,
                  std::uint32_t, std::uint32_t) override {
        events.emplace_back("wide-copy");
    }
    void InsertByte(pal4::StaticCharacterBufferState& state,
                    std::uint32_t at, std::uint32_t count,
                    std::uint8_t fill) override {
        Check(at == state.multibyte_end && fill == 0u,
              "byte growth inserts zeros at former end");
        events.emplace_back("byte-grow:" + std::to_string(count));
        const auto old_capacity = state.multibyte_begin == 0u
            ? 0u : state.multibyte_end - state.multibyte_begin;
        state.multibyte_begin = grown_byte;
        state.multibyte_end = grown_byte + old_capacity + count;
    }
    void WriteWide(std::uint32_t address,
                   std::uint16_t value) override {
        Check(value == 0u, "wide conversion writes terminator");
        wide_terminator = address;
        events.emplace_back("wide-term");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(value == 0u, "UTF-8 conversion writes terminator");
        byte_terminator = address;
        events.emplace_back("byte-term");
    }

    std::uint32_t current_wide = 0x1000u;
};

void ExistingBuffersAvoidGrowth() {
    pal4::StaticCharacterBufferState state;
    state.wide_begin = 0x1000u;
    state.wide_end = 0x1100u;
    state.multibyte_begin = 0x2000u;
    state.multibyte_end = 0x2080u;
    Runtime runtime;
    Check(pal4::CeguiTextConverter::ConvertToUtf8(
              "abc", state, runtime) == 0x2000u &&
              runtime.events == std::vector<std::string>{
                  "wide-query", "wide-write", "wide-term",
                  "byte-query", "byte-write", "byte-term"},
          "sufficient global buffers avoid both insertion helpers");
    Check(runtime.wide_terminator == 0x1006u &&
              runtime.byte_terminator == 0x2005u &&
              state.wide_begin == 0x1000u &&
              state.multibyte_begin == 0x2000u,
          "conversion terminators follow counted payloads");
}

void BothBuffersGrowToTwiceRequiredLength() {
    pal4::StaticCharacterBufferState state;
    state.wide_begin = 0x1000u;
    state.wide_end = 0x1004u;
    state.multibyte_begin = 0x2000u;
    state.multibyte_end = 0x2002u;
    Runtime runtime;
    Check(pal4::CeguiTextConverter::ConvertToUtf8(
              "abc", state, runtime) == 0x4000u &&
              runtime.events == std::vector<std::string>{
                  "wide-query", "wide-grow:4", "wide-write", "wide-term",
                  "byte-query", "byte-grow:8", "byte-write", "byte-term"},
          "wide and UTF-8 buffers grow by doubled target minus old capacity");
    Check(state.wide_begin == 0x3000u && state.wide_end == 0x300cu &&
              state.multibyte_begin == 0x4000u &&
              state.multibyte_end == 0x400au &&
              runtime.wide_terminator == 0x3006u &&
              runtime.byte_terminator == 0x4005u,
          "conversion reloads global pointers after both insertions");
}

void ZeroLengthAndEmbeddedNullPreserveObservedCalls() {
    pal4::StaticCharacterBufferState state;
    Runtime runtime;
    runtime.wide_count = 0;
    runtime.byte_count = 0;
    runtime.expected_source_length = 0;
    runtime.grown_wide = 0u;
    runtime.current_wide = 0u;
    Check(pal4::CeguiTextConverter::ConvertToUtf8(
              "", state, runtime) == 0u &&
              runtime.wide_terminator == 0u &&
              runtime.byte_terminator == 0u &&
              runtime.events == std::vector<std::string>{
                  "wide-query", "wide-query", "wide-term",
                  "byte-query", "byte-query", "byte-term"},
          "empty conversion still calls APIs and writes null-address terminators");
    runtime = Runtime{};
    runtime.expected_source_length = 2;
    state.wide_begin = 0x1000u;
    state.wide_end = 0x1100u;
    state.multibyte_begin = 0x2000u;
    state.multibyte_end = 0x2080u;
    Check(pal4::CeguiTextConverter::ConvertToUtf8(
              std::string_view("ab\0cd", 5), state, runtime) == 0x2000u,
          "source length stops at the first embedded NUL");
    pal4::CeguiTextConversionStub stub;
    pal4::StaticCharacterBufferState stub_state;
    Check(pal4::CeguiTextConverter::ConvertToUtf8(
              "", stub_state, stub) == 0u,
          "linkable conversion stub runs without Win32 conversion functions");
}

}  // namespace

int main() {
    ExistingBuffersAvoidGrowth();
    BothBuffersGrowToTwiceRequiredLength();
    ZeroLengthAndEmbeddedNullPreserveObservedCalls();
    std::cout << "3 CEGUI text conversion test cases passed\n";
}
