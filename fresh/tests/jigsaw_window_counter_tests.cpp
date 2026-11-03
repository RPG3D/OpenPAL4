#include "pal4/jigsaw_window_counter.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IJigsawWindowCounterRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    static constexpr std::uint32_t window = 0x2000u;
    std::vector<std::string> calls;
    std::int32_t count = 3;
    std::uint8_t gate = 0u;
    bool has_format = true;
    bool throw_set_text = false;
    std::string format_used;
    std::int32_t value_formatted = 0;
    std::u32string text;
    std::uint32_t result_success = 99u;
    std::string released;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == frame + 0x18cu, "timer gate offset");
        calls.emplace_back("gate");
        return gate;
    }
    std::int32_t ReadSignedWord(std::uint32_t address) override {
        Check(address == frame + 0x74u, "counter offset");
        calls.emplace_back("count");
        return count;
    }
    void WriteSignedWord(std::uint32_t address,
                         std::int32_t value) override {
        Check(address == frame + 0x74u, "counter write offset");
        count = value;
        calls.emplace_back("write");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == frame + 0x188u, "step text window offset");
        calls.emplace_back("window");
        return window;
    }
    std::optional<std::string_view> TriggerFormat(
        std::uint32_t id) override {
        Check(id == 0xf5u, "trigger format ID");
        calls.emplace_back("trigger");
        if (!has_format) return std::nullopt;
        return "steps: %d";
    }
    std::string FormatText(std::string_view format,
                           std::int32_t value) override {
        format_used = format;
        value_formatted = value;
        calls.emplace_back("format");
        return "step text";
    }
    std::string ConvertToUtf8(std::string_view source) override {
        Check(source == "step text", "conversion input");
        calls.emplace_back("utf8");
        return "\xe6\xad\xa5\xe6\x95\xb0";
    }
    void SetText(std::uint32_t target,
                 std::u32string_view value) override {
        Check(target == window, "text window");
        calls.emplace_back("set-text");
        if (throw_set_text) throw std::runtime_error("set-text failed");
        text = value;
    }
    void ShowResult(std::uint32_t owner,
                    std::uint32_t success) override {
        Check(owner == frame, "result frame");
        result_success = success;
        calls.emplace_back("result");
    }
    std::uint32_t ReleaseFormatted(std::string& source) override {
        released = source;
        calls.emplace_back("release");
        return 0x1234abu;
    }
};

void CheckUpdate() {
    Runtime active;
    Check(pal4::JigsawWindowCounter::Update(
              Runtime::frame, active) == 0x1234abu &&
              active.value_formatted == 3 &&
              active.format_used == "steps: %d" &&
              active.text == U"\u6b65\u6570" &&
              active.released == "step text" &&
              active.result_success == 99u &&
              active.calls == (std::vector<std::string>{
                  "count", "trigger", "format", "utf8", "window",
                  "set-text", "count", "release"}),
          "counter text update and cleanup");

    Runtime exhausted;
    exhausted.count = 0;
    exhausted.has_format = false;
    Check(pal4::JigsawWindowCounter::Update(
              Runtime::frame, exhausted) == 0x1234abu &&
              exhausted.format_used.empty() &&
              exhausted.value_formatted == 0 &&
              exhausted.result_success == 0u &&
              exhausted.calls == (std::vector<std::string>{
                  "count", "trigger", "format", "utf8", "window",
                  "set-text", "count", "result", "release"}),
          "zero counter shows failure after text update");

    Runtime failing;
    failing.throw_set_text = true;
    bool caught = false;
    try {
        (void)pal4::JigsawWindowCounter::Update(Runtime::frame, failing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failing.calls.back() == "release" &&
              failing.result_success == 99u,
          "failed text update releases formatted buffer");
}

void CheckDecrement() {
    Runtime gated;
    gated.gate = 7u;
    Check(pal4::JigsawWindowCounter::Decrement(
              Runtime::frame, gated) == 7u && gated.count == 3 &&
              gated.calls == (std::vector<std::string>{"gate"}),
          "disabled timer returns gate byte");

    for (const auto start : {4, 0, -2}) {
        Runtime active;
        active.count = start;
        Check(pal4::JigsawWindowCounter::Decrement(
                  Runtime::frame, active) == 0xabu &&
                  active.count == (start > 0 ? start - 1 : start) &&
                  active.value_formatted == active.count &&
                  active.calls[0] == "gate" &&
                  active.calls[1] == "count" &&
                  active.calls[2] == "write" &&
                  active.calls.back() == "release",
              "signed timer decrement and tail update");
        Check(active.result_success == (start == 0 ? 0u : 99u),
              "failure appears only at zero");
    }
}

}  // namespace

int main() {
    try {
        CheckUpdate();
        CheckDecrement();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
