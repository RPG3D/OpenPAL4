#include "pal4/puzzle_window_counter.h"

#include <array>
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

struct Runtime final : pal4::IPuzzleWindowCounterRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    static constexpr std::uint32_t window = 0x2000u;
    std::vector<std::string> calls;
    std::int32_t count = 11;
    std::uint8_t gate = 0u;
    bool has_format = true;
    bool throw_set_text = false;
    std::array<float, 4> colour{};
    std::string sound;
    std::uint32_t sound_mode = 99u;
    std::string used_format;
    std::int32_t formatted_count = 0;
    std::u32string text;
    std::uint32_t result_success = 99u;
    std::string released;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == frame + 0x190u, "finish flag offset");
        calls.emplace_back("gate");
        return gate;
    }
    std::int32_t ReadSignedWord(std::uint32_t address) override {
        Check(address == frame + 0x74u, "signed counter offset");
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
    void SetTextColours(
        std::uint32_t target,
        const std::array<float, 4>& value) override {
        Check(target == window, "colour window");
        colour = value;
        calls.emplace_back("colour");
    }
    void PlaySound(std::string_view file,
                   std::uint32_t mode) override {
        sound = file;
        sound_mode = mode;
        calls.emplace_back("sound");
    }
    std::optional<std::string_view> TriggerFormat(
        std::uint32_t id) override {
        Check(id == 0xe9u, "counter trigger ID");
        calls.emplace_back("trigger");
        if (!has_format) return std::nullopt;
        return "remaining: %d";
    }
    std::string FormatText(std::string_view format,
                           std::int32_t value) override {
        used_format = format;
        formatted_count = value;
        calls.emplace_back("format");
        return "formatted";
    }
    std::string ConvertToUtf8(std::string_view input) override {
        Check(input == "formatted", "UTF-8 conversion input");
        calls.emplace_back("utf8");
        return "\xe5\x89\xa9\xe4\xbd\x99";
    }
    void SetText(std::uint32_t target,
                 std::u32string_view value) override {
        Check(target == window, "counter text window");
        calls.emplace_back("text");
        if (throw_set_text) throw std::runtime_error("text failed");
        text = value;
    }
    void ShowResult(std::uint32_t owner,
                    std::uint32_t success) override {
        Check(owner == frame, "failure frame");
        result_success = success;
        calls.emplace_back("result");
    }
    std::uint32_t ReleaseFormatted(std::string& source) override {
        released = source;
        calls.emplace_back("release");
        return 0x1234abu;
    }
};

void CheckDisplay() {
    Runtime gated;
    gated.gate = 7u;
    Check(pal4::PuzzleWindowCounter::UpdateDisplay(
              Runtime::frame, gated) == 7u &&
              gated.calls == (std::vector<std::string>{"gate"}),
          "finished puzzle returns gate byte");

    for (const auto start : {11, 10, 1, 0, -2}) {
        Runtime runtime;
        runtime.count = start;
        runtime.has_format = start != -2;
        Check(pal4::PuzzleWindowCounter::UpdateDisplay(
                  Runtime::frame, runtime) == 0xabu &&
                  runtime.colour == (start <= 10
                      ? std::array<float, 4>{1.0f, 0.0f, 0.0f, 1.0f}
                      : std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f}) &&
                  runtime.sound == (start > 0 && start <= 10
                                       ? "WA25.wav" : "") &&
                  runtime.formatted_count == start &&
                  runtime.used_format == (start == -2 ? "" : "remaining: %d") &&
                  runtime.text == U"\u5269\u4f59" &&
                  runtime.released == "formatted" &&
                  runtime.result_success == (start == 0 ? 0u : 99u),
              "counter colour, warning sound and formatted text");
        if (!runtime.sound.empty())
            Check(runtime.sound_mode == 1u, "warning sound mode");
        Check(runtime.calls.front() == "gate" &&
                  runtime.calls.back() == "release" &&
                  runtime.calls[3] == "colour",
              "colour precedes formatting and cleanup follows result");
    }

    Runtime failing;
    failing.throw_set_text = true;
    bool caught = false;
    try {
        (void)pal4::PuzzleWindowCounter::UpdateDisplay(
            Runtime::frame, failing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failing.calls.back() == "release",
          "text exception releases formatted buffer");
}

void CheckDecrement() {
    Runtime gated;
    gated.gate = 3u;
    Check(pal4::PuzzleWindowCounter::Decrement(
              Runtime::frame, gated) == 3u && gated.count == 11 &&
              gated.calls == (std::vector<std::string>{"gate"}),
          "timer gate skips decrement");
    for (const auto start : {2, 1, 0, -2}) {
        Runtime runtime;
        runtime.count = start;
        Check(pal4::PuzzleWindowCounter::Decrement(
                  Runtime::frame, runtime) == 0xabu &&
                  runtime.count == (start > 0 ? start - 1 : start) &&
                  runtime.formatted_count == runtime.count &&
                  runtime.calls[0] == "gate" &&
                  runtime.calls[1] == "count" &&
                  runtime.calls[2] == "write" &&
                  runtime.calls[3] == "gate" &&
                  runtime.result_success == (start == 1 || start == 0
                                                 ? 0u : 99u),
              "timer decrements positives and tail updates display");
    }
}

}  // namespace

int main() {
    try {
        CheckDisplay();
        CheckDecrement();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
