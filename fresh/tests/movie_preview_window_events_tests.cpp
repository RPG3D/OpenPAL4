#include "pal4/movie_preview_window_events.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MoviePreviewWindowEventRuntimeStub {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    bool fail_close{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t ConstructBaseEvent() override {
        calls.emplace_back("construct-event");
        return 0xa000u;
    }
    void DestroyBaseEvent(std::uint32_t event) override {
        Check(event == 0xa000u, "temporary event destroyed");
        calls.emplace_back("destroy-event");
    }
    void Close(pal4::MoviePreviewWindowState&,
               std::uint32_t object, std::uint32_t event) override {
        Check(object == 0x6000u && event == 0xa000u,
              "member pointer adjusted to containing movie window");
        calls.emplace_back("close");
        if (fail_close) throw std::runtime_error("close");
    }
    void HandleItemClick(pal4::MoviePreviewWindowState&,
                         std::uint32_t selected) override {
        calls.emplace_back("click:" + std::to_string(selected));
    }
    void UpdateStaticInfo(pal4::MoviePreviewWindowState&,
                          std::uint32_t selected) override {
        calls.emplace_back("info:" + std::to_string(selected));
    }
};

pal4::MoviePreviewWindowState State() {
    pal4::MoviePreviewWindowState state{};
    for (std::uint32_t i = 0u; i < 13u; ++i)
        state.Write(0x2cu + i * 4u, 0x2100u + i * 0x100u);
    state.Write(0x68u, 0x5000u);
    return state;
}

void CheckKey(std::uint32_t code, std::uint32_t index,
              std::string expected_action,
              std::uint8_t expected_result = 1u) {
    auto state = State();
    Runtime runtime;
    runtime.words = {{0x8010u, code}, {0x5118u, index}};
    const auto result = pal4::MoviePreviewWindowEvents::OnMemberEvent(
        state, 0x6028u, 0x8000u, runtime);
    Check(result == expected_result, "key event result");
    const auto expected_size = expected_action.empty() ? 2u : 3u;
    Check(runtime.calls.size() == expected_size &&
              runtime.calls[0] == "read:32784" &&
              runtime.calls[1] == "read:20760" &&
              (expected_action.empty() ||
               runtime.calls[2] == expected_action),
          "key event reads mask index and uses mapped image");
}

void CheckNavigation() {
    CheckKey(28u, 2u, "click:8960");
    CheckKey(28u, 13u, "");
    CheckKey(203u, 2u, "info:8704");
    CheckKey(203u, 0u, "");
    CheckKey(205u, 2u, "info:9216");
    CheckKey(205u, 12u, "");
    CheckKey(200u, 5u, "info:8704");
    CheckKey(200u, 3u, "");
    CheckKey(208u, 5u, "info:10752");
    CheckKey(208u, 9u, "");
    CheckKey(999u, 2u, "", 0u);
}

void CheckCloseEvent() {
    auto state = State();
    Runtime runtime;
    runtime.words[0x8010u] = 1u;
    Check(pal4::MoviePreviewWindowEvents::OnMemberEvent(
              state, 0x6028u, 0x8000u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "read:32784", "construct-event", "close", "destroy-event"},
          "event one closes through temporary CEGUI event");
    runtime.calls.clear();
    runtime.fail_close = true;
    try {
        (void)pal4::MoviePreviewWindowEvents::OnMemberEvent(
            state, 0x6028u, 0x8000u, runtime);
        Check(false, "close failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "destroy-event",
              "temporary event released on close failure");
    }
}

}  // namespace

int main() {
    CheckNavigation();
    CheckCloseEvent();
    std::cout << "Movie preview window event tests passed\n";
}
