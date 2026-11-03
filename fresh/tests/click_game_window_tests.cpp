#include "pal4/click_game_window.h"

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

struct Binding {
    pal4::ClickGameWindowEvent event;
    std::uint32_t callback;
    bool operator==(const Binding&) const = default;
};

struct Runtime final : pal4::IClickGameWindowRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t root = 0x4000u;
    static constexpr std::uint32_t image = 0x5000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {object + 0x24u, root}};
    std::string name = "ClickGameWindow/";
    std::string lookup;
    std::uint32_t layout = 0u;
    std::uint32_t allocated_bytes = 0u;
    std::uint32_t next_callback = 0x6000u;
    std::uint32_t next_subscription = 0x8000u;
    std::uint32_t released = 0u;
    std::vector<Binding> bindings;
    std::vector<std::string> calls;
    float width = 0.625f;
    float saved_width = 0.0f;

    void LoadLayout(std::uint32_t address,
                    std::uint32_t layout_name) override {
        Check(address == object, "layout object");
        layout = layout_name;
        calls.emplace_back("layout");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    void WriteFloat(std::uint32_t address, float value) override {
        Check(address == object + 0x2cu, "width offset");
        saved_width = value;
        calls.emplace_back("width-write");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == object + 0x24u, "root offset");
        calls.emplace_back("root-read");
        return words.at(address);
    }
    std::string_view WindowName(std::uint32_t address) override {
        Check(address == object, "name object");
        calls.emplace_back("name");
        return name;
    }
    std::uint32_t GetWindow(std::uint32_t address,
                            std::string_view window_name) override {
        Check(address == object, "lookup object");
        lookup = window_name;
        calls.emplace_back("lookup");
        return image;
    }
    float GetWidth(std::uint32_t window) override {
        Check(window == image, "width window");
        calls.emplace_back("get-width");
        return width;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocated_bytes = bytes;
        calls.emplace_back("allocate");
        if (next_callback == 0u) return 0u;
        const auto callback = next_callback;
        next_callback += 0x10u;
        return callback;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            pal4::ClickGameWindowEvent event,
                            std::uint32_t callback) override {
        Check(window == root, "subscription root");
        bindings.push_back({event, callback});
        calls.emplace_back("subscribe");
        return next_subscription;
    }
    void ReleaseSubscription(std::uint32_t subscription) override {
        released = subscription;
        calls.emplace_back("release");
    }
};

void CheckConstruction() {
    Runtime runtime;
    Check(pal4::ClickGameWindow::Construct(
              Runtime::object, 0x1234u, runtime) == Runtime::object &&
              runtime.layout == 0x1234u &&
              runtime.words.at(Runtime::object) == 0x843e20u &&
              runtime.words.at(Runtime::object + 0x30u) == 0u &&
              runtime.lookup == "ClickGameWindow/ImgBar" &&
              runtime.words.at(Runtime::object + 0x28u) == Runtime::image &&
              runtime.saved_width == runtime.width,
          "layout, image bar and width initialization");
    Check(runtime.allocated_bytes == 0xcu &&
              runtime.bindings == (std::vector<Binding>{
                  {pal4::ClickGameWindowEvent::KeyUp, 0x6000u},
                  {pal4::ClickGameWindowEvent::MouseClick, 0x6010u}}) &&
              runtime.words.at(0x6000u) == 0x843e18u &&
              runtime.words.at(0x6004u) == 0x49f6d0u &&
              runtime.words.at(0x6008u) == Runtime::object &&
              runtime.words.at(0x6010u) == 0x843e18u &&
              runtime.words.at(0x6014u) == 0x49f6a0u &&
              runtime.words.at(0x6018u) == Runtime::object &&
              runtime.released == 0x8000u,
          "callback layout, event order and subscription release");
    Check(runtime.calls == (std::vector<std::string>{
              "layout", "write:4096", "write:4144", "name", "lookup",
              "write:4136", "get-width", "width-write", "allocate",
              "write:24580", "write:24584", "write:24576", "root-read",
              "subscribe", "release", "allocate", "write:24596",
              "write:24600", "write:24592", "root-read", "subscribe",
              "release"}),
          "constructor preserves observable operation order");
}

void CheckMissingCallbacksAndSubscriptions() {
    Runtime runtime;
    runtime.name.clear();
    runtime.next_callback = 0u;
    runtime.next_subscription = 0u;
    Check(pal4::ClickGameWindow::Construct(
              Runtime::object, 0u, runtime) == Runtime::object &&
              runtime.lookup == "ImgBar" &&
              runtime.bindings == (std::vector<Binding>{
                  {pal4::ClickGameWindowEvent::KeyUp, 0u},
                  {pal4::ClickGameWindowEvent::MouseClick, 0u}}) &&
              runtime.released == 0u,
          "empty name and callback allocation failure still subscribe");
}

}  // namespace

int main() {
    try {
        CheckConstruction();
        CheckMissingCallbacksAndSubscriptions();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
