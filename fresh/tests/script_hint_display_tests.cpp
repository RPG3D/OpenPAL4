#include "pal4/script_hint_display.h"

#include "pal4/preview_text_codec.h"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptHintDisplayRuntime {
    std::uint32_t source = 0x5000u;
    std::string utf8 = "Hi";
    bool npos = false;
    std::u32string shown;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x3004u, "script text payload offset");
        calls.emplace_back("source"); return source;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty"); return 0x4000u;
    }
    std::string ConvertToUtf8(std::uint32_t text) override {
        Check(text == (source == 0u ? 0x4000u : source),
              "conversion source with empty fallback");
        calls.emplace_back("convert"); return utf8;
    }
    std::uint32_t EncodedSize(std::string_view text) override {
        Check(text == std::string_view(utf8).substr(0u, utf8.find('\0')),
              "encoding sees NUL-terminated UTF-8");
        calls.emplace_back("size");
        return npos ? std::numeric_limits<std::uint32_t>::max() :
            static_cast<std::uint32_t>(
                pal4::PreviewTextCodec::DecodeUtf8(text).size());
    }
    std::u32string Encode(std::string_view text,
                          std::uint32_t size) override {
        const auto decoded = pal4::PreviewTextCodec::DecodeUtf8(text);
        Check(size == decoded.size(), "CEGUI encoded codepoint count");
        calls.emplace_back("encode"); return decoded;
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("frames"); return 0x1000u;
    }
    std::uint32_t CreateHintWindow(std::uint32_t frames) override {
        Check(frames == 0x1000u, "hint window frame manager");
        calls.emplace_back("window"); return 0x2000u;
    }
    void ShowHint(std::uint32_t window, std::u32string_view text,
                  float x, float y, float duration,
                  std::uint32_t parent, std::uint32_t flag) override {
        Check(window == 0x2000u && x == 0.25f && y == 0.75f &&
                  duration == 2.5f && parent == 0u && flag == 0u,
              "hint geometry and fixed arguments");
        calls.emplace_back("show"); shown = text;
    }
};

void ShowHintBranches() {
    Runtime normal;
    pal4::ScriptHintDisplay::Show(0x3000u, 0.25f, 0.75f, normal);
    Check(normal.shown == U"Hi" &&
              normal.calls == std::vector<std::string>{
                  "source", "convert", "size", "encode", "frames",
                  "window", "show"},
          "ordinary hint conversion and display order");
    Runtime empty;
    empty.source = 0u;
    empty.utf8.clear();
    pal4::ScriptHintDisplay::Show(0x3000u, 0.25f, 0.75f, empty);
    Check(empty.shown.empty() &&
              empty.calls == std::vector<std::string>{
                  "source", "empty", "convert", "size", "encode",
                  "frames", "window", "show"},
          "null string payload uses empty text");
    Runtime unicode;
    unicode.utf8 = std::string("\xe6\xb1\x89\0ignored", 11u);
    pal4::ScriptHintDisplay::Show(0x3000u, 0.25f, 0.75f, unicode);
    Check(unicode.shown == U"\u6c49", "UTF-8 encoding stops at NUL");
}

void LengthError() {
    Runtime runtime;
    runtime.npos = true;
    bool thrown = false;
    try {
        pal4::ScriptHintDisplay::Show(0x3000u, 0.25f, 0.75f, runtime);
    } catch (const std::length_error&) {
        thrown = true;
    }
    Check(thrown && runtime.calls == std::vector<std::string>{
              "source", "convert", "size"},
          "npos encoded size throws before window creation");
}

}  // namespace

int main() {
    try {
        ShowHintBranches();
        LengthError();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
