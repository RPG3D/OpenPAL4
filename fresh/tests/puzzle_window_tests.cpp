#include "pal4/puzzle_window.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Binding {
    std::uint32_t window;
    pal4::PuzzleWindowEvent event;
    std::uint32_t callback;
    bool operator==(const Binding&) const = default;
};

struct Runtime final : pal4::IPuzzleWindowRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t board = 0x3000u;
    static constexpr std::uint32_t step = 0x4000u;
    static constexpr std::uint32_t text = 0x5000u;
    static constexpr std::uint32_t close = 0x6000u;
    std::string prefix = "Puzzle/";
    std::uint32_t loaded_layout = 0u;
    std::uint32_t initialized_puzzle = 0u;
    std::uint32_t string_address = 0u;
    std::string string_value;
    std::uint32_t formatted_window = 0u;
    std::uint32_t horizontal = 0u;
    std::uint32_t vertical = 0u;
    std::uint32_t next_callback = 0x7000u;
    std::uint32_t subscription = 0x8000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> names;
    std::vector<Binding> bindings;
    std::vector<std::uint32_t> released;
    std::vector<std::string> calls;

    void LoadLayout(std::uint32_t address,
                    std::uint32_t layout) override {
        Check(address == object, "layout frame");
        loaded_layout = layout;
        calls.emplace_back("layout");
    }
    void InitializePuzzle(std::uint32_t address) override {
        initialized_puzzle = address;
        calls.emplace_back("puzzle");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    void SetString(std::uint32_t address,
                   std::string_view value) override {
        string_address = address;
        string_value = value;
        calls.emplace_back("string");
    }
    std::string_view WindowName(std::uint32_t address) override {
        Check(address == object, "prefix frame");
        calls.emplace_back("name");
        return prefix;
    }
    std::uint32_t GetWindow(std::uint32_t address,
                            std::string_view name) override {
        Check(address == object, "window frame");
        names.emplace_back(name);
        calls.emplace_back("lookup");
        if (name.ends_with("StaticBoard")) return board;
        if (name.ends_with("StaticStep")) return step;
        if (name.ends_with("StaticText")) return text;
        Check(name.ends_with("BtnClose"), "window suffix");
        return close;
    }
    void SetStaticTextFormatting(std::uint32_t window,
                                 std::uint32_t x,
                                 std::uint32_t y) override {
        formatted_window = window;
        horizontal = x;
        vertical = y;
        calls.emplace_back("format");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "callback bytes");
        calls.emplace_back("allocate");
        if (next_callback == 0u) return 0u;
        const auto result = next_callback;
        next_callback += 0x20u;
        return result;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            pal4::PuzzleWindowEvent event,
                            std::uint32_t callback) override {
        bindings.push_back({window, event, callback});
        calls.emplace_back("subscribe");
        return subscription;
    }
    void ReleaseSubscription(std::uint32_t token) override {
        released.push_back(token);
        calls.emplace_back("release");
    }
};

void CheckConstructor() {
    Runtime runtime;
    Check(pal4::PuzzleWindow::Construct(
              Runtime::object, 0x2222u, runtime) == Runtime::object &&
              runtime.loaded_layout == 0x2222u &&
              runtime.initialized_puzzle == Runtime::object + 0x2cu &&
              runtime.string_address == Runtime::object + 0x194u &&
              runtime.string_value == "puzzleStep" &&
              runtime.words.at(Runtime::object) == 0x843eb4u &&
              runtime.words.at(Runtime::object + 0x28u) == 0x843e9cu &&
              runtime.words.at(Runtime::object + 0x184u) == Runtime::board &&
              runtime.words.at(Runtime::object + 0x188u) == Runtime::step &&
              runtime.words.at(Runtime::object + 0x18cu) == Runtime::text &&
              runtime.formatted_window == Runtime::text &&
              runtime.horizontal == 2u && runtime.vertical == 2u,
          "puzzle window layout, strings, controls and formatting");
    Check(runtime.names == (std::vector<std::string>{
              "Puzzle/StaticBoard", "Puzzle/StaticStep",
              "Puzzle/StaticText", "Puzzle/BtnClose"}) &&
              runtime.bindings == (std::vector<Binding>{
                  {Runtime::board, pal4::PuzzleWindowEvent::RenderingEnded,
                   0x7000u},
                  {Runtime::board, pal4::PuzzleWindowEvent::MouseButtonUp,
                   0x7020u},
                  {Runtime::close, pal4::PuzzleWindowEvent::Clicked,
                   0x7040u}}) &&
              runtime.released == (std::vector<std::uint32_t>{
                  0x8000u, 0x8000u, 0x8000u}),
          "control lookup and subscription order");
    for (const auto [callback, handler] : {
             std::pair{0x7000u, 0x4a1250u},
             std::pair{0x7020u, 0x4a12b0u},
             std::pair{0x7040u, 0x4a14a0u}}) {
        Check(runtime.words.at(callback) == 0x843e94u &&
                  runtime.words.at(callback + 8u) == handler &&
                  runtime.words.at(callback + 0xcu) == 0u &&
                  runtime.words.at(callback + 0x10u) == Runtime::object,
              "callback layout");
    }
    Check(runtime.calls == (std::vector<std::string>{
              "layout", "write", "puzzle", "string", "write", "write",
              "name", "lookup", "write", "lookup", "write", "lookup",
              "write", "format", "allocate", "write", "write",
              "write", "write", "subscribe", "release", "allocate",
              "write", "write", "write", "write", "subscribe", "release",
              "lookup", "allocate", "write", "write", "write", "write",
              "subscribe", "release"}),
          "constructor operation order");
}

void CheckMissingCallbacks() {
    Runtime runtime;
    runtime.prefix.clear();
    runtime.next_callback = 0u;
    runtime.subscription = 0u;
    Check(pal4::PuzzleWindow::Construct(
              Runtime::object, 0u, runtime) == Runtime::object &&
              runtime.names == (std::vector<std::string>{
                  "StaticBoard", "StaticStep", "StaticText", "BtnClose"}) &&
              runtime.bindings == (std::vector<Binding>{
                  {Runtime::board, pal4::PuzzleWindowEvent::RenderingEnded,
                   0u},
                  {Runtime::board, pal4::PuzzleWindowEvent::MouseButtonUp,
                   0u},
                  {Runtime::close, pal4::PuzzleWindowEvent::Clicked,
                   0u}}) && runtime.released.empty(),
          "null callback and subscription paths");
}

}  // namespace

int main() {
    try {
        CheckConstructor();
        CheckMissingCallbacks();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
