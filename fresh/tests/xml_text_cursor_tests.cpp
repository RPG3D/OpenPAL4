#include "pal4/xml_text_cursor.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IXmlTextCursorRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> writes;
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        writes.push_back(address);
        words[address] = value;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void SetText(const std::uint32_t start,
                 const std::vector<std::uint8_t>& text) {
        for (std::size_t index = 0; index < text.size(); ++index)
            bytes[start + static_cast<std::uint32_t>(index)] = text[index];
        bytes[start + static_cast<std::uint32_t>(text.size())] = 0u;
    }
};

Runtime MakeRuntime(const std::vector<std::uint8_t>& text) {
    Runtime runtime;
    runtime.words[0x1000u] = 3u;
    runtime.words[0x1004u] = 2u;
    runtime.words[0x1008u] = 0x2000u;
    runtime.words[0x100cu] = 4u;
    runtime.SetText(0x2000u, text);
    return runtime;
}

void TestColumnsAndNewlines() {
    auto runtime = MakeRuntime({'a', 'b', '\t', 'c'});
    pal4::XmlTextCursor::Advance(0x1000u, 0x2004u, 0, runtime);
    Check(runtime.words[0x1000u] == 3u &&
              runtime.words[0x1004u] == 9u &&
              runtime.words[0x1008u] == 0x2004u &&
              runtime.writes == std::vector<std::uint32_t>{
                  0x1000u, 0x1004u, 0x1008u},
          "tab advances to next configured column stop");
    runtime = MakeRuntime({'a', '\r', '\n', 'b', '\n', '\r', 'c'});
    pal4::XmlTextCursor::Advance(0x1000u, 0x2007u, 0, runtime);
    Check(runtime.words[0x1000u] == 5u &&
              runtime.words[0x1004u] == 1u &&
              runtime.words[0x1008u] == 0x2007u,
          "CRLF and LFCR pairs each advance one line");
    runtime = MakeRuntime({'\r', 'x'});
    pal4::XmlTextCursor::Advance(0x1000u, 0x2001u, 0, runtime);
    Check(runtime.words[0x1000u] == 4u &&
              runtime.words[0x1004u] == 0u &&
              runtime.words[0x1008u] == 0x2001u,
          "single carriage return does not consume an unrelated next byte");
}

void TestUtf8AndBom() {
    auto runtime = MakeRuntime({0xc2u, 0xa2u, 0xe2u, 0x82u, 0xacu,
                                0xf0u, 0x9fu, 0x98u, 0x80u, 'x'});
    pal4::XmlTextCursor::Advance(0x1000u, 0x200au, 1, runtime);
    Check(runtime.words[0x1004u] == 6u &&
              runtime.words[0x1008u] == 0x200au,
          "UTF-8 mode counts four sequences as four columns");
    runtime = MakeRuntime({0xc2u, 0xa2u, 0xe2u, 0x82u, 0xacu,
                           0xf0u, 0x9fu, 0x98u, 0x80u, 'x'});
    pal4::XmlTextCursor::Advance(0x1000u, 0x200au, 0, runtime);
    Check(runtime.words[0x1004u] == 12u,
          "byte mode counts every UTF-8 byte separately");
    runtime = MakeRuntime({0xefu, 0xbbu, 0xbfu,
                           0xefu, 0xbfu, 0xbeu,
                           0xefu, 0xbfu, 0xbfu,
                           0xefu, 0x80u, 0x80u});
    pal4::XmlTextCursor::Advance(0x1000u, 0x200cu, 1, runtime);
    Check(runtime.words[0x1004u] == 3u &&
              runtime.words[0x1008u] == 0x200cu,
          "three recognized BOM shapes skip columns, other EF adds one");
    runtime = MakeRuntime({0xf5u, 0xc1u, 0xf4u, 0x80u, 0x80u, 0x80u});
    pal4::XmlTextCursor::Advance(0x1000u, 0x2006u, 1, runtime);
    Check(runtime.words[0x1004u] == 5u,
          "lead table treats F5 and C1 as one byte, F4 as four");
}

void TestEarlyExit() {
    auto runtime = MakeRuntime({'a', 0u, 'b'});
    pal4::XmlTextCursor::Advance(0x1000u, 0x2003u, 0, runtime);
    Check(runtime.words[0x1004u] == 2u &&
              runtime.words[0x1008u] == 0x2000u &&
              runtime.writes.empty(),
          "NUL before end discards pending line, column and cursor changes");
    runtime = MakeRuntime({0xefu, 0u});
    pal4::XmlTextCursor::Advance(0x1000u, 0x2002u, 1, runtime);
    Check(runtime.writes.empty(),
          "incomplete EF sequence exits without spinning or committing");
    runtime = MakeRuntime({'x'});
    runtime.words[0x100cu] = 0u;
    pal4::XmlTextCursor::Advance(0x1000u, 0x2001u, 0, runtime);
    Check(runtime.writes.empty() && runtime.words[0x1008u] == 0x2000u,
          "nonpositive tab width prevents processing");
    runtime.words[0x100cu] = 4u;
    pal4::XmlTextCursor::Advance(0x1000u, 0x2000u, 0, runtime);
    Check(runtime.words[0x1008u] == 0x2000u &&
              runtime.writes.size() == 3u,
          "empty range still commits unchanged cursor fields");
}

void TestSkipWhitespaceAndBom() {
    using Cursor = pal4::XmlTextCursor;
    const std::uint8_t empty[]{0u};
    Check(Cursor::SkipWhitespaceAndBom(nullptr, 1) == nullptr &&
              Cursor::SkipWhitespaceAndBom(empty, 1) == nullptr,
          "null and initially empty text return null");
    const std::uint8_t spaces[]{' ', '\t', '\r', '\n', 0u};
    Check(Cursor::SkipWhitespaceAndBom(spaces, 0) == spaces + 4u,
          "whitespace-only input returns the terminator location");
    const std::uint8_t marked[]{
        0xefu, 0xbbu, 0xbfu, ' ',
        0xefu, 0xbfu, 0xbeu, '\n',
        0xefu, 0xbfu, 0xbfu, 'x', 0u};
    Check(Cursor::SkipWhitespaceAndBom(marked, 1) == marked + 11u &&
              Cursor::SkipWhitespaceAndBom(marked, 0) == marked,
          "UTF-8 mode skips all three recognized markers and whitespace");
    const std::uint8_t marker_only[]{0xefu, 0xbbu, 0xbfu, 0u};
    Check(Cursor::SkipWhitespaceAndBom(marker_only, 1) ==
              marker_only + 3u,
          "marker-only input returns the terminator, not null");
    const std::uint8_t unknown[]{' ', 0xefu, 0x80u, 0x80u, 0u};
    Check(Cursor::SkipWhitespaceAndBom(unknown, 1) == unknown + 1u,
          "unrecognized EF sequence is preserved after leading whitespace");
}

}  // namespace

int main() {
    TestColumnsAndNewlines();
    TestUtf8AndBom();
    TestEarlyExit();
    TestSkipWhitespaceAndBom();
}
