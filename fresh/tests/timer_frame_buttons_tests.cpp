#include "pal4/timer_frame_buttons.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ITimerFrameButtonsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> names;
    std::vector<std::uint32_t> trigger_ids;
    std::vector<std::tuple<std::uint32_t, std::uint32_t,
                           std::uint32_t>> subscriptions;
    std::vector<std::uint32_t> released_connections;
    std::vector<std::uint32_t> constructed_functions;
    std::vector<std::uint32_t> freed;
    std::map<std::uint32_t, std::string> text_by_window;
    std::uint32_t allocations = 0u;
    std::uint32_t end_count = 0u;
    bool fail_allocation = false;
    bool fail_callback_constructor = false;
    bool fail_delegate_text = false;

    std::uint32_t BeginName(std::uint32_t frame) override {
        Check(frame == 0x1000u, "name starts from frame prefix");
        return 0x9000u;
    }
    std::uint32_t ResetName(std::uint32_t buffer,
                            std::uint32_t frame,
                            std::string_view suffix) override {
        Check(buffer == 0x9000u && frame == 0x1000u,
              "one local name buffer is reused");
        names.emplace_back(suffix);
        return 0x9001u + static_cast<std::uint32_t>(names.size());
    }
    std::uint32_t EndName(std::uint32_t buffer) override {
        Check(buffer == 0x9000u, "local name buffer released");
        ++end_count;
        return 0x8888u;
    }
    std::uint32_t FindWindow(std::uint32_t name) override {
        Check(name == 0x9001u + names.size(),
              "lookup uses latest composed name");
        return 0x1100u +
               static_cast<std::uint32_t>(names.size() - 1u) * 0x100u;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "each callback allocates 24 bytes");
        const auto index = allocations++;
        return fail_allocation ? 0u : 0x2000u + index * 0x18u;
    }
    std::uint32_t ConstructCallback(std::uint32_t allocation,
                                    std::uint32_t function,
                                    std::uint32_t frame) override {
        Check(allocation == 0x2000u +
                  (constructed_functions.size() * 0x18u) &&
                  frame == 0x1000u,
              "first two callback constructors receive allocation and owner");
        constructed_functions.push_back(function);
        if (fail_callback_constructor) {
            throw std::runtime_error("callback constructor");
        }
        return allocation;
    }
    void Free(std::uint32_t address) override { freed.push_back(address); }
    std::uint32_t Subscribe(std::uint32_t window,
                            std::uint32_t event,
                            std::uint32_t callback) override {
        subscriptions.emplace_back(window, event, callback);
        return 0x4000u +
               static_cast<std::uint32_t>(subscriptions.size());
    }
    void ReleaseConnection(std::uint32_t connection) override {
        released_connections.push_back(connection);
    }
    std::uint32_t TriggerText(std::uint32_t trigger_id) override {
        trigger_ids.push_back(trigger_id);
        if (trigger_id == 278u) return 0x5000u;
        if (trigger_id == 189u) return 0u;
        if (trigger_id == 30u) return 0x5100u;
        Check(false, "unexpected trigger id");
        return 0u;
    }
    std::string_view ToUtf8(std::uint32_t text) override {
        if (text == 0x5000u) return "settle";
        if (text == pal4::TimerFrameButtons::kEmptyText) {
            if (fail_delegate_text) throw std::runtime_error("utf8");
            return "";
        }
        if (text == 0x5100u) return "exit";
        Check(false, "unexpected text pointer");
        return {};
    }
    void SetText(std::uint32_t window,
                 std::string_view text) override {
        text_by_window[window] = text;
    }
    std::array<std::uint32_t, 2> Position(
        std::uint32_t window) override {
        return {window + 1u, window + 2u};
    }
    std::uint32_t HeightBits(std::uint32_t window) override {
        Check(window == 0x1400u, "panel height read from panel window");
        return 0x41200000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
};

void SeedEvents(Runtime& runtime) {
    runtime.words[pal4::TimerFrameButtons::kClickedEvent] = 0x3000u;
    runtime.words[pal4::TimerFrameButtons::kKeyDownEvent] = 0x3010u;
    runtime.words[0x1024u] = 0x1600u;
}

void CheckHappyPath() {
    Runtime runtime;
    SeedEvents(runtime);
    Check(pal4::TimerFrameButtons::Initialize(0x1000u, runtime) ==
              0x8888u && runtime.end_count == 1u,
          "button initialization releases one local name buffer");
    Check(runtime.names == std::vector<std::string>{
                               "BtnSettle", "BtnDelegate", "BtnExit",
                               "PanelButton", "imgBottom"} &&
              runtime.trigger_ids ==
                  std::vector<std::uint32_t>{278u, 189u, 30u},
          "resource suffixes and GameDB trigger IDs preserve IDA order");
    Check(runtime.words[0x1058u] == 0x1100u &&
              runtime.words[0x105cu] == 0x1200u &&
              runtime.words[0x1060u] == 0x1300u &&
              runtime.words[0x1040u] == 0x1101u &&
              runtime.words[0x1044u] == 0x1102u &&
              runtime.words[0x1048u] == 0x1201u &&
              runtime.words[0x104cu] == 0x1202u &&
              runtime.words[0x1050u] == 0x1301u &&
              runtime.words[0x1054u] == 0x1302u,
          "three windows and saved Vector2 positions use expected offsets");
    Check(runtime.words[0x1068u] == 0x1400u &&
              runtime.words[0x106cu] == 0x41200000u &&
              runtime.words[0x1064u] == 0x1500u,
          "panel, height bits and bottom image fields");
    Check(runtime.text_by_window[0x1100u] == "settle" &&
              runtime.text_by_window[0x1200u].empty() &&
              runtime.text_by_window[0x1300u] == "exit",
          "trigger text conversion includes null text fallback");
    Check(runtime.allocations == 4u &&
              runtime.constructed_functions ==
                  std::vector<std::uint32_t>{
                      pal4::TimerFrameButtons::kSettleClick,
                      pal4::TimerFrameButtons::kDelegateClick} &&
              runtime.words[0x2030u] ==
                  pal4::TimerFrameButtons::kCallbackVtable &&
              runtime.words[0x2038u] ==
                  pal4::TimerFrameButtons::kExitClick &&
              runtime.words[0x203cu] == 0u &&
              runtime.words[0x2040u] == 0x1000u &&
              runtime.words[0x2048u] ==
                  pal4::TimerFrameButtons::kCallbackVtable &&
              runtime.words[0x2050u] ==
                  pal4::TimerFrameButtons::kKeyDown,
          "constructed and directly initialized callback variants");
    Check(runtime.subscriptions ==
              std::vector<std::tuple<std::uint32_t, std::uint32_t,
                                     std::uint32_t>>{
                  {0x1100u, 0x3000u, 0x2000u},
                  {0x1200u, 0x3000u, 0x2018u},
                  {0x1300u, 0x3000u, 0x2030u},
                  {0x1600u, 0x3010u, 0x2048u}} &&
              runtime.released_connections ==
                  std::vector<std::uint32_t>{0x4001u, 0x4002u,
                                             0x4003u, 0x4004u},
          "button clicks and root keydown subscribed and released");
}

void CheckFailures() {
    Runtime no_callback;
    SeedEvents(no_callback);
    no_callback.fail_allocation = true;
    (void)pal4::TimerFrameButtons::Initialize(0x1000u, no_callback);
    Check(no_callback.allocations == 4u &&
              no_callback.constructed_functions.empty() &&
              std::get<2>(no_callback.subscriptions[0]) == 0u &&
              std::get<2>(no_callback.subscriptions[3]) == 0u,
          "allocation failure still subscribes null callbacks");

    Runtime bad_callback;
    SeedEvents(bad_callback);
    bad_callback.fail_callback_constructor = true;
    bool caught = false;
    try {
        (void)pal4::TimerFrameButtons::Initialize(
            0x1000u, bad_callback);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && bad_callback.freed ==
                        std::vector<std::uint32_t>{0x2000u} &&
              bad_callback.end_count == 1u &&
              bad_callback.subscriptions.empty(),
          "callback construction exception frees allocation and name");

    Runtime bad_text;
    SeedEvents(bad_text);
    bad_text.fail_delegate_text = true;
    caught = false;
    try {
        (void)pal4::TimerFrameButtons::Initialize(0x1000u, bad_text);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && bad_text.end_count == 1u &&
              bad_text.subscriptions.size() == 2u &&
              bad_text.released_connections.size() == 2u,
          "text conversion exception releases name after prior bindings");
}

}  // namespace

int main() {
    CheckHappyPath();
    CheckFailures();
}
