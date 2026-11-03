#include "pal4/jigsaw_window_input.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IJigsawWindowInputRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    static constexpr std::uint32_t handler = frame + 0x28u;
    static constexpr std::uint32_t event = 0x2000u;
    static constexpr std::uint32_t board = 0x3000u;

    std::unordered_map<std::uint32_t, std::uint8_t> bytes{
        {frame + 0x18cu, 0u}};
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {frame + 0x184u, board}, {event + 0x1cu, 0u},
        {event + 0x10u, 203u}, {handler + 0x44u, 1u}};
    std::vector<std::string> calls;
    std::array<float, 4> rect{10.0f, 20.0f, 30.0f, 40.0f};
    std::array<float, 2> point{3.0f, 7.0f};
    std::uint32_t result = 2u;
    bool complete = false;
    pal4::JigsawDirection direction = pal4::JigsawDirection::Left;
    std::string sound;
    std::uint32_t sound_mode = 9u;

    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("byte");
        return bytes.at(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("word");
        return words.at(address);
    }
    std::array<float, 4> GetBoardRect(std::uint32_t value) override {
        Check(value == board, "board rectangle target");
        calls.emplace_back("rect");
        return rect;
    }
    std::array<float, 2> ReadPoint(std::uint32_t address) override {
        Check(address == event + 0xcu, "mouse point offset");
        calls.emplace_back("point");
        return point;
    }
    std::uint32_t ProcessMouse(
        std::uint32_t puzzle, const std::array<float, 2>& origin,
        const std::array<float, 2>& position) override {
        Check(puzzle == frame + 0x2cu &&
                  origin == (std::array<float, 2>{30.0f, 10.0f}) &&
                  position == point, "mouse collision arguments");
        calls.emplace_back("mouse");
        return result;
    }
    std::uint32_t ProcessDirection(
        std::uint32_t puzzle, pal4::JigsawDirection value) override {
        Check(puzzle == frame + 0x2cu, "keyboard puzzle target");
        direction = value;
        calls.emplace_back("direction");
        return result;
    }
    void ResetSelection(std::uint32_t puzzle) override {
        Check(puzzle == frame + 0x2cu, "reset puzzle target");
        calls.emplace_back("reset");
    }
    void ConfirmSelection(std::uint32_t puzzle) override {
        Check(puzzle == frame + 0x2cu, "confirm puzzle target");
        calls.emplace_back("confirm");
    }
    void Redraw(std::uint32_t value) override {
        Check(value == board, "redraw board target");
        calls.emplace_back("redraw");
    }
    bool IsComplete(std::uint32_t puzzle) override {
        Check(puzzle == frame + 0x2cu, "completion puzzle target");
        calls.emplace_back("complete");
        return complete;
    }
    void ShowResult(std::uint32_t owner,
                    std::uint32_t success) override {
        Check(owner == frame && success == 1u, "success result arguments");
        calls.emplace_back("result");
    }
    std::uint32_t UpdateCounter(std::uint32_t owner) override {
        Check(owner == frame, "step counter owner");
        calls.emplace_back("counter");
        return 0u;
    }
    void PlaySound(std::string_view file,
                   std::uint32_t mode) override {
        sound = file;
        sound_mode = mode;
        calls.emplace_back("sound");
    }
    std::uint32_t Close(std::uint32_t owner,
                        std::uint32_t first,
                        std::uint32_t second) override {
        Check(owner == frame && first == 0u && second == 0u,
              "virtual close arguments");
        calls.emplace_back("close");
        return 0u;
    }
};

void CheckMouse() {
    Runtime gated;
    gated.bytes[Runtime::frame + 0x18cu] = 1u;
    Check(pal4::JigsawWindowInput::OnMouseButtonUp(
              Runtime::frame, Runtime::event, gated) == 1u &&
              gated.calls == (std::vector<std::string>{"byte"}),
          "inactive mouse path");

    Runtime other_button;
    other_button.words[Runtime::event + 0x1cu] = 1u;
    Check(pal4::JigsawWindowInput::OnMouseButtonUp(
              Runtime::frame, Runtime::event, other_button) == 1u &&
              other_button.calls == (std::vector<std::string>{"byte", "word"}),
          "mouse button gate");

    for (const auto result : {0u, 1u, 2u}) {
        Runtime active;
        active.result = result;
        Check(pal4::JigsawWindowInput::OnMouseButtonUp(
                  Runtime::frame, Runtime::event, active) == 1u,
              "mouse return");
        const auto expected = result == 2u
            ? std::vector<std::string>{"byte", "word", "word", "rect",
                                       "point", "mouse", "redraw", "complete",
                                       "counter"}
            : std::vector<std::string>{"byte", "word", "word", "rect",
                                       "point", "mouse", "redraw", "complete",
                                       "counter", "sound"};
        Check(active.calls == expected, "mouse operation order");
        if (result != 2u)
            Check(active.sound == (result == 0u ? "WA26.wav" : "WA27.wav") &&
                      active.sound_mode == 1u, "mouse sound and mode");
    }

    Runtime finished;
    finished.complete = true;
    finished.result = 0u;
    Check(pal4::JigsawWindowInput::OnMouseButtonUp(
              Runtime::frame, Runtime::event, finished) == 1u &&
              finished.calls.back() == "result" &&
              finished.sound.empty(), "completed mouse action");
}

void CheckKeyboard() {
    Runtime gated;
    gated.bytes[Runtime::frame + 0x18cu] = 1u;
    Check(pal4::JigsawWindowInput::OnKeyUp(
              Runtime::handler, Runtime::event, gated) == 1u &&
              gated.calls == (std::vector<std::string>{"byte"}),
          "inactive keyboard path");

    constexpr std::array keys{203u, 205u, 200u, 208u};
    constexpr std::array directions{
        pal4::JigsawDirection::Left, pal4::JigsawDirection::Right,
        pal4::JigsawDirection::Up, pal4::JigsawDirection::Down};
    for (std::size_t i = 0; i < keys.size(); ++i) {
        Runtime active;
        active.words[Runtime::event + 0x10u] = keys[i];
        active.result = static_cast<std::uint32_t>(i % 3u);
        Check(pal4::JigsawWindowInput::OnKeyUp(
                  Runtime::handler, Runtime::event, active) == 1u &&
                  active.direction == directions[i], "direction dispatch");
        const auto expected = active.result == 2u
            ? std::vector<std::string>{"byte", "word", "direction", "word",
                                       "redraw", "complete", "counter"}
            : std::vector<std::string>{"byte", "word", "direction", "word",
                                       "redraw", "complete", "counter", "sound"};
        Check(active.calls == expected, "direction operation order");
        if (active.result != 2u)
            Check(active.sound == (active.result == 0u ? "WA26.wav" : "WA27.wav") &&
                      active.sound_mode == 0u, "keyboard sound and mode");
    }

    Runtime escape_close;
    escape_close.words[Runtime::event + 0x10u] = 1u;
    escape_close.words[Runtime::handler + 0x44u] = 0u;
    Check(pal4::JigsawWindowInput::OnKeyUp(
              Runtime::handler, Runtime::event, escape_close) == 1u &&
              escape_close.calls == (std::vector<std::string>{
                  "byte", "word", "word", "close"}), "escape closes frame");

    for (const auto key : {1u, 28u}) {
        Runtime selected;
        selected.words[Runtime::event + 0x10u] = key;
        const auto action = key == 1u ? "reset" : "confirm";
        const auto expected = key == 1u
            ? std::vector<std::string>{"byte", "word", "word", action,
                                       "word", "redraw", "complete", "counter"}
            : std::vector<std::string>{"byte", "word", action, "word",
                                       "redraw", "complete", "counter"};
        Check(pal4::JigsawWindowInput::OnKeyUp(
                  Runtime::handler, Runtime::event, selected) == 1u &&
                  selected.calls == expected && selected.sound.empty(),
              "selection key action");
    }

    Runtime unknown;
    unknown.words[Runtime::event + 0x10u] = 99u;
    Check(pal4::JigsawWindowInput::OnKeyUp(
              Runtime::handler, Runtime::event, unknown) == 1u &&
              unknown.calls == (std::vector<std::string>{"byte", "word"}),
          "unknown key is ignored");

    Runtime finished;
    finished.complete = true;
    finished.result = 0u;
    Check(pal4::JigsawWindowInput::OnKeyUp(
              Runtime::handler, Runtime::event, finished) == 1u &&
              finished.calls.back() == "result" &&
              finished.sound.empty(), "completed keyboard action");
}

void CheckClose() {
    Runtime active;
    Check(pal4::JigsawWindowInput::OnCloseClick(
              Runtime::frame, active) == 1u &&
              active.calls == (std::vector<std::string>{"byte", "close"}),
          "close callback delegates while active");
    Runtime blocked;
    blocked.bytes[Runtime::frame + 0x18cu] = 1u;
    Check(pal4::JigsawWindowInput::OnCloseClick(
              Runtime::frame, blocked) == 1u &&
              blocked.calls == (std::vector<std::string>{"byte"}),
          "close callback gate");
}

}  // namespace

int main() {
    try {
        CheckMouse();
        CheckKeyboard();
        CheckClose();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
