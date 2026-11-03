#include "pal4/numeric_spinner.h"

#include <algorithm>
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

struct Subscription {
    std::uint32_t window;
    pal4::SpinnerMouseEvent event;
    pal4::SpinnerCallback callback;
    std::uint32_t owner;
    bool operator==(const Subscription&) const = default;
};

struct Runtime final : pal4::INumericSpinnerControlsRuntime {
    std::string name = "TradeItem1";
    std::uint32_t next_window = 0x2000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> floats;
    std::vector<std::pair<std::string, std::string>> windows;
    std::vector<std::pair<std::string, std::string>> lookups;
    std::vector<std::pair<std::uint32_t,
                          pal4::SpinnerImageState>> images;
    std::vector<Subscription> subscriptions;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> children;
    std::vector<std::string> calls;
    std::uint32_t update_item = 0u;
    std::int32_t update_value = 0;

    std::string_view ReadName(std::uint32_t address) override {
        Check(address == 0x1000u + 444u, "name address");
        calls.emplace_back("name");
        return name;
    }
    std::uint32_t CreateWindow(std::string_view type,
                               std::string_view name_value) override {
        windows.emplace_back(type, name_value);
        calls.emplace_back("window:" + std::to_string(windows.size()));
        const auto result = next_window;
        next_window += 0x100u;
        return result;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        calls.emplace_back("float:" + std::to_string(address));
    }
    void SetStandardImagery(std::uint32_t button,
                            bool enabled) override {
        Check(!enabled && (button == 0x2100u || button == 0x2200u),
              "standard imagery disabled");
        calls.emplace_back("standard:" + std::to_string(button));
    }
    pal4::SpinnerImage FindImage(std::string_view imageset,
                                 std::string_view image) override {
        lookups.emplace_back(imageset, image);
        calls.emplace_back("image:" + std::string(image));
        const bool right = image.find("you") != std::string_view::npos;
        return {static_cast<std::uint32_t>(lookups.size()),
                right ? 12.0f : 8.0f,
                right ? 6.0f : 5.0f};
    }
    void SetButtonImage(std::uint32_t button,
                        pal4::SpinnerImageState state,
                        std::uint32_t image) override {
        Check(image == lookups.size(), "image lookup forwarded");
        images.emplace_back(button, state);
        calls.emplace_back("set_image:" + std::to_string(button));
    }
    void SetHeight(std::uint32_t window, float height) override {
        floats[window + 1u] = height;
        calls.emplace_back("height:" + std::to_string(window));
    }
    void SetWidth(std::uint32_t window, float width) override {
        floats[window + 2u] = width;
        calls.emplace_back("width:" + std::to_string(window));
    }
    void SetBlackText(std::uint32_t window) override {
        Check(window == 0x2300u, "text window colour");
        calls.emplace_back("black");
    }
    void SetFormatting(std::uint32_t window,
                       std::uint32_t horizontal,
                       std::uint32_t vertical) override {
        Check(window == 0x2300u && horizontal == 2u && vertical == 1u,
              "static text formatting");
        calls.emplace_back("format");
    }
    void Subscribe(std::uint32_t window,
                   pal4::SpinnerMouseEvent event,
                   pal4::SpinnerCallback callback,
                   std::uint32_t owner) override {
        subscriptions.push_back({window, event, callback, owner});
        calls.emplace_back("subscribe:" + std::to_string(subscriptions.size()));
    }
    void AddChild(std::uint32_t parent,
                  std::uint32_t child) override {
        children.emplace_back(parent, child);
        calls.emplace_back("child:" + std::to_string(child));
    }
    void UpdateValue(std::uint32_t item,
                     std::int32_t value) override {
        update_item = item;
        update_value = value;
        calls.emplace_back("update");
    }
};

std::size_t Find(const std::vector<std::string>& calls,
                 const std::string& target) {
    const auto it = std::find(calls.begin(), calls.end(), target);
    Check(it != calls.end(), "missing call");
    return static_cast<std::size_t>(it - calls.begin());
}

void CheckCreate(std::string name, std::int32_t initial_value) {
    Runtime runtime;
    runtime.name = std::move(name);
    runtime.words[0x1000u + 440u] =
        static_cast<std::uint32_t>(initial_value);
    pal4::NumericSpinner::CreateControls(0x1000u, runtime);

    Check(runtime.windows ==
              (std::vector<std::pair<std::string, std::string>>{
                  {"DefaultWindow", runtime.name + "Frame"},
                  {"OiramLook/Button", runtime.name + "ButtonLeft"},
                  {"OiramLook/Button", runtime.name + "ButtonRight"},
                  {"OiramLook/StaticText", runtime.name + "StaticText"}}),
          "window types, names and order");
    Check(runtime.words[0x1000u + 436u] == 0x2000u &&
              runtime.words[0x1000u + 424u] == 0x2100u &&
              runtime.words[0x1000u + 428u] == 0x2200u &&
              runtime.words[0x1000u + 432u] == 0x2300u,
          "window fields");
    Check(runtime.lookups ==
              (std::vector<std::pair<std::string, std::string>>{
                  {"gy0", "sanjiaozuo3"}, {"gy0", "sanjiaozuo2"},
                  {"gy0", "sanjiaozuo2"}, {"gy0", "sanjiaozuo1"},
                  {"gy0", "sanjiaoyou3"}, {"gy0", "sanjiaoyou2"},
                  {"gy0", "sanjiaoyou2"}, {"gy0", "sanjiaoyou1"}}),
          "image resource sequence");
    using State = pal4::SpinnerImageState;
    Check(runtime.images ==
              (std::vector<std::pair<std::uint32_t, State>>{
                  {0x2100u, State::Normal}, {0x2100u, State::Pushed},
                  {0x2100u, State::Disabled}, {0x2100u, State::Hover},
                  {0x2200u, State::Normal}, {0x2200u, State::Pushed},
                  {0x2200u, State::Disabled}, {0x2200u, State::Hover}}),
          "button image states");
    Check(runtime.floats[0x2101u] == 5.0f &&
              runtime.floats[0x2102u] == 8.0f &&
              runtime.floats[0x2201u] == 6.0f &&
              runtime.floats[0x2202u] == 12.0f &&
              runtime.floats[0x2301u] == 6.0f &&
              runtime.floats[0x2302u] == 15.0f &&
              runtime.floats[0x1000u + 460u] == 39.0f &&
              runtime.floats[0x1000u + 464u] == 8.0f,
          "hover dimensions and item pair");
    using Event = pal4::SpinnerMouseEvent;
    using Callback = pal4::SpinnerCallback;
    Check(runtime.subscriptions == std::vector<Subscription>{
              {0x2100u, Event::Clicked, Callback::LeftClick, 0x1000u},
              {0x2200u, Event::Clicked, Callback::RightClick, 0x1000u},
              {0x2100u, Event::ButtonDown, Callback::LeftDown, 0x1000u},
              {0x2200u, Event::ButtonDown, Callback::RightDown, 0x1000u},
              {0x2100u, Event::ButtonUp, Callback::ButtonUp, 0x1000u},
              {0x2200u, Event::ButtonUp, Callback::ButtonUp, 0x1000u}},
          "six callback subscriptions");
    Check(runtime.children ==
              (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x2000u, 0x2200u}, {0x2000u, 0x2100u},
                  {0x2000u, 0x2300u}}) &&
              runtime.update_item == 0x1000u &&
              runtime.update_value == initial_value &&
              Find(runtime.calls, "subscribe:6") <
                  Find(runtime.calls, "child:8704") &&
              Find(runtime.calls, "child:8960") <
                  Find(runtime.calls, "update"),
          "child order and final value refresh");
}

}  // namespace

int main() {
    try {
        CheckCreate("TradeItem1", 42);
        CheckCreate("", -5);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
