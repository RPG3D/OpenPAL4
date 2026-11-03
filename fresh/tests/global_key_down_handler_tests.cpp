#include "pal4/global_key_down_handler.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGlobalKeyDownRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t mode_object = 0u;
    std::uint32_t mode_result = 0u;
    std::uint32_t attribute = 0u;
    bool timer_visible = false;
    bool video_playing = false;
    std::uint8_t handler_result = 0x7du;
    std::uint32_t dispatched_frame = 0u;
    std::uint32_t attribute_value = 0u;
    std::uint32_t invoked_handler = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t CurrentModeObject() override {
        calls.emplace_back("mode_object");
        return mode_object;
    }
    std::uint32_t QueryMode(std::uint32_t mode) override {
        calls.emplace_back("query_mode");
        Check(mode == mode_object, "mode object forwarded");
        return mode_result;
    }
    std::uint32_t UiManager() override {
        calls.emplace_back("manager");
        return 0x2000u;
    }
    void CaptureScreenshot() override { calls.emplace_back("screenshot"); }
    bool IsTimerFrameVisible(std::uint32_t manager) override {
        calls.emplace_back("timer_visible");
        Check(manager == 0x2000u, "timer manager");
        return timer_visible;
    }
    void DispatchTimerEvent(std::uint32_t manager, std::uint32_t event_key,
                            std::uint32_t active,
                            std::uint32_t frame) override {
        calls.emplace_back("dispatch");
        Check(manager == 0x2000u &&
              event_key == pal4::GlobalKeyDownHandler::kTimerEventKey &&
              active == 1u, "timer event arguments");
        dispatched_frame = frame;
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::uint32_t key) override {
        calls.emplace_back("find_frame");
        Check(manager == 0x2000u &&
              key == pal4::GlobalKeyDownHandler::kFrameKey,
              "frame lookup arguments");
        return 0x9000u;
    }
    std::uint32_t CreateDocument() override {
        calls.emplace_back("create_xml");
        return 0x5000u;
    }
    std::uint32_t FindAttribute(std::uint32_t document,
                                std::string_view name) override {
        calls.emplace_back("find_attribute");
        Check(document == 0x5000u && name == "Frame",
              "frame XML attribute lookup");
        return attribute;
    }
    void SetAttributeInt(std::uint32_t attr,
                         std::uint32_t value) override {
        calls.emplace_back("set_attribute");
        Check(attr == attribute, "existing XML attribute");
        attribute_value = value;
    }
    void AddAttributeInt(std::uint32_t document, std::string_view name,
                         std::uint32_t value) override {
        calls.emplace_back("add_attribute");
        Check(document == 0x5000u && name == "Frame",
              "new XML attribute arguments");
        attribute_value = value;
    }
    void ChangeGameState(std::uint32_t state,
                         std::uint32_t document) override {
        calls.emplace_back("change_state");
        Check(state == 10u && document == 0x5000u,
              "game state switch arguments");
    }
    void DestroyDocument(std::uint32_t document) override {
        calls.emplace_back("destroy_xml");
        Check(document == 0x5000u, "XML cleanup object");
    }
    std::uint32_t BinkPlayer() override {
        calls.emplace_back("bink_player");
        return 0x3000u;
    }
    bool BinkIsPlaying(std::uint32_t state) override {
        calls.emplace_back("bink_playing");
        Check(state == 0x3004u, "video state offset");
        return video_playing;
    }
    void CloseVideo(std::uint32_t player) override {
        calls.emplace_back("close_video");
        Check(player == 0x3000u, "video player argument");
    }
    std::uint8_t InvokeLastHandler(std::uint32_t handler,
                                   std::uint32_t event) override {
        calls.emplace_back("invoke_last");
        Check(event == 0x4000u, "forwarded event");
        invoked_handler = handler;
        return handler_result;
    }
};

Runtime Base(std::uint32_t key) {
    Runtime runtime;
    runtime.words[0x4010u] = key;
    runtime.words[0x1004u] = 0x7000u;
    runtime.words[0x1008u] = 0x7008u;
    runtime.words[0x7000u] = 0x8000u;
    runtime.words[0x7004u] = 0x8004u;
    runtime.words[0x2144u] = 0xaaaa5555u;
    return runtime;
}

bool Has(const Runtime& runtime, std::string_view call) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), call) !=
           runtime.calls.end();
}

std::size_t Position(const Runtime& runtime, std::string_view call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "expected service call absent");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckScreenshot() {
    auto runtime = Base(66u);
    runtime.mode_object = 0x6000u;
    Check(pal4::GlobalKeyDownHandler::Process(0x1000u, 0x4000u,
                                                runtime) == 0u,
          "screenshot key result");
    Check(runtime.calls == std::vector<std::string>{
              "mode_object", "query_mode", "read:16400", "manager",
              "screenshot"},
          "screenshot branch service order");
}

void CheckReturnState(std::uint32_t mode, bool visible,
                      std::uint32_t attribute) {
    auto runtime = Base(67u);
    runtime.mode_object = 0x6000u;
    runtime.mode_result = mode;
    runtime.timer_visible = visible;
    runtime.attribute = attribute;
    Check(pal4::GlobalKeyDownHandler::Process(0x1000u, 0x4000u,
                                                runtime) == 1u,
          "return key consumed with mode object");
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "query_mode") == 2,
          "return key queries mode before and inside branch");
    const bool dispatch = (mode == 1u && visible) ||
                          mode == 2u || mode == 4u || mode == 5u;
    Check(Has(runtime, "dispatch") == dispatch,
          "mode-based timer dispatch");
    if (dispatch)
        Check(runtime.dispatched_frame == 0xaaaa5555u,
              "timer frame from manager +324");
    const bool change = mode == 1u && visible;
    Check(Has(runtime, "change_state") == change,
          "mode-one visible timer changes game state");
    if (change) {
        Check(runtime.attribute_value == 0x9000u,
              "found frame stored in XML");
        Check(Has(runtime, attribute ? "set_attribute" : "add_attribute"),
              "existing or new Frame attribute branch");
        Check(Position(runtime, "dispatch") < Position(runtime, "create_xml") &&
              Position(runtime, "create_xml") < Position(runtime, "find_frame") &&
              Position(runtime, "find_frame") < Position(runtime, "change_state") &&
              Position(runtime, "change_state") < Position(runtime, "destroy_xml"),
              "timer and XML state transition order");
    }
    Check(!Has(runtime, "invoke_last"), "return key bypasses other handlers");
}

void CheckFallbacks() {
    auto no_mode = Base(67u);
    Check(pal4::GlobalKeyDownHandler::Process(0x1000u, 0x4000u,
                                                no_mode) == 0x7du &&
          no_mode.invoked_handler == 0x8004u,
          "return key without mode falls through to last handler");

    auto video = Base(1u);
    video.video_playing = true;
    Check(pal4::GlobalKeyDownHandler::Process(0x1000u, 0x4000u,
                                                video) == 0u,
          "playing video consumes key");
    Check(std::count(video.calls.begin(), video.calls.end(),
                     "bink_player") == 2 &&
          Has(video, "close_video") && !Has(video, "invoke_last"),
          "video closes after second player lookup");

    auto idle_video = Base(1u);
    Check(pal4::GlobalKeyDownHandler::Process(0x1000u, 0x4000u,
                                                idle_video) == 0x7du &&
          Has(idle_video, "invoke_last"),
          "idle video forwards key to last handler");

    auto ordinary = Base(9u);
    Check(pal4::GlobalKeyDownHandler::Process(0x1000u, 0x4000u,
                                                ordinary) == 0x7du &&
          ordinary.invoked_handler == 0x8004u,
          "ordinary key forwards to last handler");

    auto empty = Base(9u);
    empty.words[0x1008u] = empty.words[0x1004u];
    Check(pal4::GlobalKeyDownHandler::Process(0x1000u, 0x4000u,
                                                empty) == 0u &&
          !Has(empty, "invoke_last"),
          "empty handler array returns zero");
}

}  // namespace

int main() {
    try {
        CheckScreenshot();
        CheckReturnState(0u, false, 0u);
        CheckReturnState(1u, false, 0u);
        CheckReturnState(1u, true, 0u);
        CheckReturnState(1u, true, 0x6000u);
        for (const auto mode : {2u, 4u, 5u})
            CheckReturnState(mode, false, 0u);
        CheckReturnState(3u, false, 0u);
        CheckFallbacks();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
