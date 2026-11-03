#include "pal4/game_info_window.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameInfoWindowRuntime {
    std::vector<std::string> calls;
    std::vector<std::string> names;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t allocation = 0x7000u;
    std::uint32_t connection = 0x8000u;
    std::uint32_t frame = 0x9000u;
    std::uint32_t subscribed_window = 0u;
    std::uint32_t subscribed_callback = 0u;
    std::uint32_t event_tree = 0u;
    std::uint32_t event_key = 0u;
    std::array<std::uint32_t, 2> event_data{};
    std::uint32_t show_name = 0u;
    std::uint32_t show_event = 0u;
    std::uint32_t updated_frame = 0u;
    std::int32_t money = -123;
    std::array<std::string, 2> trigger_text{"first", "second"};
    std::vector<std::uint32_t> text_windows;
    std::vector<std::u32string> text_values;
    std::uint32_t released_formatted = 0u;
    std::vector<std::uint32_t> formatted_seconds;
    std::vector<std::string> released_times;
    bool visible = false;
    std::uint32_t initialized_address = 0u;
    std::uint8_t initialized_byte = 0u;
    std::string assigned_text;
    int fail_lookup = -1;
    bool fail_assign = false;
    bool fail_subscribe = false;
    bool fail_set_text = false;

    void LoadLayout(std::uint32_t object, std::uint32_t name) override {
        calls.emplace_back("layout");
        Check(object == 0x1000u && name == 0x1234u, "layout arguments");
    }
    void InitializeText(std::uint32_t address, std::uint8_t byte) override {
        calls.emplace_back("init-text");
        initialized_address = address;
        initialized_byte = byte;
    }
    void AssignText(std::uint32_t address, std::string_view text) override {
        calls.emplace_back("assign-text");
        Check(address == 0x1040u, "text address");
        if (fail_assign) throw std::runtime_error("assign");
        assigned_text = text;
    }
    void DestroyText(std::uint32_t address) override {
        calls.emplace_back("destroy-text");
        Check(address == 0x1040u, "destroy text address");
    }
    std::uint32_t DestroyBase(std::uint32_t object) override {
        calls.emplace_back("destroy-base");
        Check(object == 0x1000u, "destroy base address");
        return 0x1000u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        if (address == 0x102cu || address == 0x902cu)
            updated_frame = address - 0x2cu;
        return words.at(address);
    }
    std::string_view WindowName(std::uint32_t object) override {
        calls.emplace_back("window-name");
        Check(object == 0x1000u, "window name object");
        return "gameInfo/";
    }
    std::uint32_t GetWindow(std::string_view name) override {
        calls.emplace_back("get-window");
        if (static_cast<int>(names.size()) == fail_lookup)
            throw std::runtime_error("lookup");
        names.emplace_back(name);
        return 0x5000u + static_cast<std::uint32_t>(names.size());
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.emplace_back("allocate");
        Check(size == 12u, "callback size");
        return allocation;
    }
    std::uint32_t SubscribeClicked(std::uint32_t window,
                                   std::uint32_t callback) override {
        calls.emplace_back("subscribe");
        subscribed_window = window;
        subscribed_callback = callback;
        if (fail_subscribe) throw std::runtime_error("subscribe");
        return connection;
    }
    void ReleaseConnection(std::uint32_t value) override {
        calls.emplace_back("release-connection");
        Check(value == connection, "released connection");
    }
    std::uint32_t ReleaseNameBuffer() override {
        calls.emplace_back("release-name");
        return 0x55u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x3000u;
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::uint32_t name) override {
        calls.emplace_back("find-frame");
        Check(manager == 0x3000u && name == 0x8b139cu, "frame lookup");
        return frame;
    }
    bool HandleShow(std::uint32_t name, std::uint32_t event) override {
        calls.emplace_back("handle-show");
        show_name = name;
        show_event = event;
        return false;
    }
    void ProcessUiEvent(std::uint32_t tree, std::uint32_t key,
                        const std::array<std::uint32_t, 2>& data) override {
        calls.emplace_back("process-event");
        event_tree = tree;
        event_key = key;
        event_data = data;
    }
    std::int32_t InventoryMoney() override {
        calls.emplace_back("money");
        return money;
    }
    std::string TriggerText(std::uint32_t id) override {
        calls.emplace_back("trigger");
        Check(id == 0x63u || id == 0x64u, "trigger ID");
        return trigger_text[id - 0x63u];
    }
    std::string ConvertToUtf8(std::string_view text) override {
        calls.emplace_back("utf8");
        return std::string(text);
    }
    void SetText(std::uint32_t window, std::u32string_view text) override {
        calls.emplace_back("set-text");
        if (fail_set_text) throw std::runtime_error("set text");
        text_windows.push_back(window);
        text_values.emplace_back(text);
    }
    std::uint32_t ReleaseFormatted(const std::string& text) override {
        calls.emplace_back("release-formatted");
        Check(text == std::to_string(money), "formatted money cleanup");
        ++released_formatted;
        return 0x99u;
    }
    void SetParent(std::uint32_t object, bool value,
                   std::uint32_t parent) override {
        calls.emplace_back("set-parent");
        Check(object == 0x1000u && parent == 0x4000u, "parent arguments");
        visible = value;
    }
    bool IsVisible(std::uint32_t object) override {
        calls.emplace_back("is-visible");
        Check(object == 0x1000u, "visibility object");
        return visible;
    }
    std::uint32_t ElapsedSeconds() override {
        calls.emplace_back("elapsed");
        return 72u;
    }
    std::uint32_t ClockOffsetSeconds() override {
        calls.emplace_back("clock-offset");
        return 3600u;
    }
    std::string FormatSeconds(std::uint32_t seconds) override {
        calls.emplace_back("format-seconds");
        formatted_seconds.push_back(seconds);
        return std::to_string(seconds);
    }
    std::uint8_t ReleaseTimeString(const std::string& text) override {
        calls.emplace_back("release-time");
        released_times.push_back(text);
        return 0x44u;
    }
};

void CheckLifecycle() {
    using Window = pal4::GameInfoWindow;
    Runtime runtime;
    Check(Window::Construct(0x1000u, 0x1234u, runtime) == 0x1000u,
          "constructor return");
    Check(runtime.initialized_address == 0x1040u &&
              runtime.initialized_byte == 0x34u &&
              runtime.assigned_text == "updateTime" &&
              runtime.words.at(0x1000u) == Window::kVtable,
          "constructor text and vtable");
    const std::vector<std::string> expected{
        "gameInfo/btnHelp", "gameInfo/staticMoney", "gameInfo/staticTime",
        "gameInfo/staticTimeInfo", "gameInfo/staticGameTime",
        "gameInfo/staticGameTimeInfo"};
    Check(runtime.names == expected, "six control names and order");
    for (std::uint32_t i = 0; i < 6u; ++i)
        Check(runtime.words.at(0x1028u + 4u * i) == 0x5001u + i,
              "control offset");
    Check(runtime.words.at(0x7000u) == Window::kHelpCallbackVtable &&
              runtime.words.at(0x7004u) == 0x4a6b60u &&
              runtime.words.at(0x7008u) == 0x1000u &&
              runtime.subscribed_window == 0x5001u &&
              runtime.subscribed_callback == 0x7000u,
          "help callback and subscription");
    Check(runtime.calls.at(0) == "layout" &&
              runtime.calls.at(1) == "init-text" &&
              runtime.calls.at(2) == "assign-text" &&
              runtime.calls.at(3) == "write" &&
              runtime.calls.back() == "release-name",
          "constructor call order");
    Check(Window::Destroy(0x1000u, runtime) == 0x1000u &&
              runtime.calls.at(runtime.calls.size() - 2) == "destroy-text" &&
              runtime.calls.back() == "destroy-base",
          "destructor order");
}

void CheckFailures() {
    using Window = pal4::GameInfoWindow;
    Runtime assign;
    assign.fail_assign = true;
    try { (void)Window::Construct(0x1000u, 0x1234u, assign); }
    catch (const std::runtime_error&) {}
    Check(assign.calls.at(assign.calls.size() - 2) == "destroy-text" &&
              assign.calls.back() == "destroy-base",
          "text assignment failure cleanup");
    Runtime lookup;
    lookup.fail_lookup = 3;
    try { (void)Window::Construct(0x1000u, 0x1234u, lookup); }
    catch (const std::runtime_error&) {}
    Check(lookup.calls.at(lookup.calls.size() - 3) == "release-name" &&
              lookup.calls.at(lookup.calls.size() - 2) == "destroy-text" &&
              lookup.calls.back() == "destroy-base",
          "control lookup failure cleanup");
    Runtime null_callback;
    null_callback.allocation = 0u;
    null_callback.connection = 0u;
    (void)Window::Construct(0x1000u, 0x1234u, null_callback);
    Check(null_callback.subscribed_callback == 0u &&
              null_callback.calls.back() == "release-name",
          "null callback and connection");
    Runtime subscribe;
    subscribe.fail_subscribe = true;
    try { (void)Window::Construct(0x1000u, 0x1234u, subscribe); }
    catch (const std::runtime_error&) {}
    Check(subscribe.calls.at(subscribe.calls.size() - 3) == "release-name" &&
              subscribe.calls.at(subscribe.calls.size() - 2) == "destroy-text" &&
              subscribe.calls.back() == "destroy-base",
          "subscription failure cleanup");
}

void CheckEvents() {
    using Window = pal4::GameInfoWindow;
    Runtime runtime;
    runtime.words[0x3144u] = 0x77u;
    runtime.words[0x4560u] = 0x66u;
    Check(Window::OnHelpButtonClick(runtime) == 1u &&
              runtime.event_tree == 0x3104u &&
              runtime.event_key == Window::kHelpEventKey &&
              runtime.event_data == (std::array<std::uint32_t, 2>{1u, 0x77u}) &&
              runtime.calls == (std::vector<std::string>{
                  "manager", "read", "manager", "process-event"}),
          "help event payload and manager order");
    runtime.calls.clear();
    Check(!Window::HandleShow(0x4560u, runtime) &&
              runtime.show_name == Window::kNamePointer &&
              runtime.show_event == 0x4560u,
          "show callback forwards name and event");
    runtime.calls.clear();
    Check(Window::SetHelpTipsId(0x4560u, runtime) == 1u &&
              runtime.words.at(0x3144u) == 0x66u &&
              runtime.calls == (std::vector<std::string>{
                  "manager", "find-frame", "manager", "read", "write"}),
          "help ID callback order and destination");
    runtime.calls.clear();
    runtime.words[0x902cu] = 0x6001u;
    runtime.words[0x9030u] = 0x6002u;
    runtime.words[0x9038u] = 0x6003u;
    Check(Window::HandleUpdate(runtime) && runtime.updated_frame == 0x9000u &&
              runtime.text_windows ==
                  (std::vector<std::uint32_t>{0x6001u, 0x6002u, 0x6003u}) &&
              runtime.text_values ==
                  (std::vector<std::u32string>{U"-123", U"first", U"second"}) &&
              runtime.released_formatted == 1u &&
              runtime.calls.at(0) == "manager" &&
              runtime.calls.at(1) == "find-frame" &&
              runtime.calls.back() == "release-formatted",
          "update callback with frame");
    runtime.calls.clear();
    runtime.frame = 0u;
    runtime.updated_frame = 0u;
    Check(Window::HandleUpdate(runtime) && runtime.updated_frame == 0u &&
              runtime.calls == (std::vector<std::string>{
                  "manager", "find-frame"}),
          "update callback without frame");
}

void CheckRefreshText() {
    using Window = pal4::GameInfoWindow;
    Runtime runtime;
    runtime.words[0x102cu] = 0x6001u;
    runtime.words[0x1030u] = 0x6002u;
    runtime.words[0x1038u] = 0x6003u;
    runtime.trigger_text = {"", "\xc3\xa9\xe4\xb8\xad"};
    Check(Window::UpdateUi(0x1000u, runtime) == 0x99u &&
              runtime.text_values ==
                  (std::vector<std::u32string>{U"-123", U"", U"\u00e9\u4e2d"}) &&
              runtime.released_formatted == 1u,
          "empty and multibyte trigger text");
    runtime.calls.clear();
    runtime.fail_set_text = true;
    bool caught = false;
    try { (void)Window::UpdateUi(0x1000u, runtime); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls.back() == "release-formatted" &&
              runtime.released_formatted == 2u,
          "formatted buffer cleanup after CEGUI failure");
}

void CheckVisibilityAndClock() {
    using Window = pal4::GameInfoWindow;
    Runtime runtime;
    runtime.words[0x102cu] = 0x6001u;
    runtime.words[0x1030u] = 0x6002u;
    runtime.words[0x1034u] = 0x6004u;
    runtime.words[0x1038u] = 0x6003u;
    runtime.words[0x103cu] = 0x6005u;
    Check(Window::SetVisible(0x1000u, false, 0x4000u, runtime) == 0u &&
              runtime.calls == (std::vector<std::string>{"set-parent"}),
          "hidden window skips text and time");
    runtime.calls.clear();
    Check(Window::UpdateClock(0x1000u, runtime) == 0u &&
              runtime.calls == (std::vector<std::string>{"is-visible"}),
          "clock hidden gate");
    runtime.calls.clear();
    Check(Window::SetVisible(0x1000u, true, 0x4000u, runtime) == 0x99u &&
              runtime.visible &&
              runtime.text_windows == (std::vector<std::uint32_t>{
                  0x6001u, 0x6002u, 0x6003u, 0x6004u, 0x6005u}) &&
              runtime.text_values == (std::vector<std::u32string>{
                  U"-123", U"first", U"second", U"3672", U"72"}) &&
              runtime.formatted_seconds ==
                  (std::vector<std::uint32_t>{3672u, 72u}) &&
              runtime.released_times ==
                  (std::vector<std::string>{"72", "3672"}) &&
              runtime.released_formatted == 1u &&
              runtime.calls.back() == "release-formatted",
          "visible window updates money, triggers and clock in order");

    runtime.calls.clear();
    runtime.fail_set_text = true;
    bool caught = false;
    try { (void)Window::UpdateClock(0x1000u, runtime); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.released_times.size() == 4u &&
              runtime.released_times.at(2) == "72" &&
              runtime.released_times.at(3) == "3672",
          "clock text failure releases both formatted strings");
    runtime.calls.clear();
    caught = false;
    try { (void)Window::SetVisible(0x1000u, true, 0x4000u, runtime); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls.back() == "release-formatted" &&
              runtime.released_formatted == 2u,
          "visible refresh failure releases money buffer");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckFailures();
        CheckEvents();
        CheckRefreshText();
        CheckVisibilityAndClock();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
