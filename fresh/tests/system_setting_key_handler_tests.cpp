#include "pal4/system_setting_key_handler.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemSettingKeyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t last_frame = 0u;
    std::uint32_t last_window = 0u;
    std::uint8_t forwarded_result = 7u;

    Runtime() {
        words[0x6000u + 16u] = 1u;
        words[0x4028u + 144u] = 0x5000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void HideFrame(std::uint32_t frame, std::uint32_t first,
                   std::uint32_t second) override {
        calls.emplace_back("hide");
        Check(frame == 0x4000u && first == 0u && second == 0u,
              "exit hides owning frame through virtual visibility slot");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x7000u;
    }
    void ProcessUiEvent(std::uint32_t dispatcher,
                        std::uint32_t event,
                        std::uint32_t first,
                        std::uint32_t second) override {
        calls.emplace_back("event");
        Check(dispatcher == 0x7000u + 260u && event == 0x8a80acu &&
              first == 1u && second == 0u,
              "exit event address and payload");
    }
    std::uint8_t ForwardKey(std::uint32_t member,
                            std::uint32_t event) override {
        calls.emplace_back("forward");
        Check(member == 0x4028u && event == 0x6000u,
              "non-exit event forwarded to list handler");
        return forwarded_result;
    }
    void HandleMouseEvent(std::uint32_t window) override {
        calls.emplace_back("mouse");
        last_window = window;
    }
    void SelectLoadData(std::uint32_t frame) override {
        calls.emplace_back("load");
        last_frame = frame;
    }
    void ShowSystemSettings(std::uint32_t frame) override {
        calls.emplace_back("settings");
        last_frame = frame;
    }
    void ConfirmQuit(std::uint32_t frame) override {
        calls.emplace_back("quit");
        last_frame = frame;
    }
};

}  // namespace

int main() {
    Runtime exit;
    Check(pal4::SystemSettingKeyHandler::OnExit(
          0x4028u, 0x6000u, exit) == 1u &&
          exit.calls == std::vector<std::string>{
              "read", "hide", "manager", "event"},
          "exit event hides frame then notifies manager");

    Runtime forwarded;
    forwarded.words[0x6010u] = 9u;
    Check(pal4::SystemSettingKeyHandler::OnExit(
          0x4028u, 0x6000u, forwarded) == 7u &&
          forwarded.calls == std::vector<std::string>{"read", "forward"},
          "other key returns underlying handler result");

    for (std::uint32_t code = 2u; code <= 5u; ++code) {
        Runtime runtime;
        runtime.words[0x6010u] = code;
        Check(pal4::SystemSettingKeyHandler::OnGameMenuKey(
              0x4028u, 0x6000u, runtime) == 1u,
              "game menu recognized key");
        const char* expected = code == 2u ? "mouse" : code == 3u ? "load" :
                               code == 4u ? "settings" : "quit";
        Check(runtime.calls.back() == expected &&
              (code != 2u || runtime.last_window == 0x5000u) &&
              (code == 2u || runtime.last_frame == 0x4000u),
              "game menu action dispatch and owning frame");
        Check(runtime.calls.size() == (code == 2u ? 3u : 2u),
              "game menu does not query unrelated controls");
    }
    Runtime unknown;
    unknown.words[0x6010u] = 200u;
    Check(pal4::SystemSettingKeyHandler::OnGameMenuKey(
          0x4028u, 0x6000u, unknown) == 0u &&
          unknown.calls == std::vector<std::string>{"read"},
          "unrecognized key returns false without side effects");
}
