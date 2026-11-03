#include "pal4/jigsaw_window.h"

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
    pal4::JigsawWindowEvent event;
    std::uint32_t callback;
    bool operator==(const Binding&) const = default;
};

struct Runtime final : pal4::IJigsawWindowRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t board = 0x3000u;
    static constexpr std::uint32_t step = 0x4000u;
    static constexpr std::uint32_t close = 0x5000u;
    std::string prefix = "Jigsaw/";
    std::uint32_t loaded_layout = 0u;
    std::uint32_t initialized_puzzle = 0u;
    std::uint32_t next_callback = 0x6000u;
    std::uint32_t subscription = 0x7000u;
    std::vector<std::string> names;
    std::vector<Binding> bindings;
    std::vector<std::uint32_t> released;
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;

    void LoadLayout(std::uint32_t address,
                    std::uint32_t layout_name) override {
        Check(address == object, "layout object");
        loaded_layout = layout_name;
        calls.emplace_back("layout");
    }
    void InitializePuzzle(std::uint32_t address) override {
        initialized_puzzle = address;
        calls.emplace_back("puzzle");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::string_view WindowName(std::uint32_t address) override {
        Check(address == object, "name object");
        calls.emplace_back("name");
        return prefix;
    }
    std::uint32_t GetWindow(std::uint32_t address,
                            std::string_view name) override {
        Check(address == object, "window lookup object");
        names.emplace_back(name);
        calls.emplace_back("lookup");
        if (name.ends_with("StaticBoard")) return board;
        if (name.ends_with("StaticStep")) return step;
        Check(name.ends_with("BtnClose"), "window suffix");
        return close;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "callback size");
        calls.emplace_back("allocate");
        if (next_callback == 0u) return 0u;
        const auto result = next_callback;
        next_callback += 0x20u;
        return result;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            pal4::JigsawWindowEvent event,
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
    Check(pal4::JigsawWindow::Construct(
              Runtime::object, 0x2222u, runtime) == Runtime::object &&
              runtime.loaded_layout == 0x2222u &&
              runtime.initialized_puzzle == Runtime::object + 0x2cu &&
              runtime.words.at(Runtime::object) == 0x843e5cu &&
              runtime.words.at(Runtime::object + 0x28u) == 0x843e44u &&
              runtime.words.at(Runtime::object + 0x184u) == Runtime::board &&
              runtime.words.at(Runtime::object + 0x188u) == Runtime::step,
          "layout, puzzle subobject, vtables and windows");
    Check(runtime.names == (std::vector<std::string>{
              "Jigsaw/StaticBoard", "Jigsaw/StaticStep",
              "Jigsaw/BtnClose"}) &&
              runtime.bindings == (std::vector<Binding>{
                  {Runtime::board, pal4::JigsawWindowEvent::RenderingEnded,
                   0x6000u},
                  {Runtime::board, pal4::JigsawWindowEvent::MouseButtonUp,
                   0x6020u},
                  {Runtime::close, pal4::JigsawWindowEvent::Clicked,
                   0x6040u}}) &&
              runtime.released ==
                  (std::vector<std::uint32_t>{0x7000u, 0x7000u, 0x7000u}),
          "three controls and event subscriptions");
    for (const auto [callback, handler] : {
             std::pair{0x6000u, 0x4a1250u},
             std::pair{0x6020u, 0x4a00a0u},
             std::pair{0x6040u, 0x4a0290u}}) {
        Check(runtime.words.at(callback) == 0x843e3cu &&
                  runtime.words.at(callback + 8u) == handler &&
                  runtime.words.at(callback + 0xcu) == 0u &&
                  runtime.words.at(callback + 0x10u) == Runtime::object,
              "24-byte callback layout");
    }
    Check(runtime.calls == (std::vector<std::string>{
              "layout", "write:4136", "puzzle", "write:4096",
              "write:4136", "name", "lookup", "write:4484", "lookup",
              "write:4488", "allocate", "write:24584", "write:24588",
              "write:24592", "write:24576", "subscribe", "release",
              "allocate", "write:24616", "write:24620", "write:24624",
              "write:24608", "subscribe", "release", "lookup",
              "allocate", "write:24648", "write:24652", "write:24656",
              "write:24640", "subscribe", "release"}),
          "constructor operation order");
}

void CheckAllocationAndSubscriptionFailure() {
    Runtime runtime;
    runtime.prefix.clear();
    runtime.next_callback = 0u;
    runtime.subscription = 0u;
    Check(pal4::JigsawWindow::Construct(
              Runtime::object, 0u, runtime) == Runtime::object &&
              runtime.names == (std::vector<std::string>{
                  "StaticBoard", "StaticStep", "BtnClose"}) &&
              runtime.bindings == (std::vector<Binding>{
                  {Runtime::board, pal4::JigsawWindowEvent::RenderingEnded,
                   0u},
                  {Runtime::board, pal4::JigsawWindowEvent::MouseButtonUp,
                   0u},
                  {Runtime::close, pal4::JigsawWindowEvent::Clicked,
                   0u}}) && runtime.released.empty(),
          "allocation failure still subscribes null callbacks");
}

}  // namespace

int main() {
    try {
        CheckConstructor();
        CheckAllocationAndSubscriptionFailure();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
