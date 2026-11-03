#include "pal4/mission_window_quest_details.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMissionWindowQuestDetailsRuntime {
    std::vector<std::string> calls;
    std::string description{"quest text"};
    bool throw_on_text{};

    void SetVisible(std::uint32_t window, bool visible) override {
        Check(window == 0x8400u && !visible, "mask hidden");
        calls.emplace_back("hide");
    }
    std::string QuestDescription(std::uint32_t quest,
                                  std::uint32_t offset) override {
        Check(quest == 0x7000u && offset == 0x38u,
              "quest description field");
        calls.emplace_back("description");
        return description;
    }
    std::string ConvertToUtf8(std::string_view value) override {
        Check(value == description, "source description converted");
        calls.emplace_back("utf8");
        return "UTF8:" + std::string(value);
    }
    void SetText(std::uint32_t window,
                 std::string_view utf8_text) override {
        Check(window == 0x5c00u &&
                  utf8_text == "UTF8:" + description,
              "description control and text");
        calls.emplace_back("text");
        if (throw_on_text) throw std::runtime_error("text");
    }
    std::uint32_t LoadQuestImage(std::uint32_t quest) override {
        Check(quest == 0x7000u, "image loaded for quest");
        calls.emplace_back("load-image");
        return 0x9000u;
    }
    void SetImage(std::uint32_t window,
                  std::uint32_t image) override {
        Check(window == 0x6800u && image == 0x9000u,
              "quest image target");
        calls.emplace_back("image");
    }
};

}  // namespace

int main() {
    pal4::MissionWindowState state{};
    state.Write(0x5cu, 0x5c00u);
    state.Write(0x68u, 0x6800u);
    state.Write(0x84u, 0x8400u);
    Runtime runtime;
    pal4::MissionWindowQuestDetails::Update(state, 0u, runtime);
    Check(runtime.calls.empty(), "null quest does nothing");

    pal4::MissionWindowQuestDetails::Update(state, 0x7000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "hide", "description", "utf8", "text", "load-image",
              "image"}, "quest details operation order");

    runtime.calls.clear();
    runtime.description.clear();
    pal4::MissionWindowQuestDetails::Update(state, 0x7000u, runtime);
    Check(runtime.calls.size() == 6u,
          "empty description still updates text and image");

    runtime.calls.clear();
    runtime.throw_on_text = true;
    try {
        pal4::MissionWindowQuestDetails::Update(state, 0x7000u, runtime);
        Check(false, "text error propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "hide", "description", "utf8", "text"},
              "image is not loaded before text update succeeds");
    }
    std::cout << "Mission window quest details tests passed\n";
}
