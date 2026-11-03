#include "pal4/pal_test_window_question_flow.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PalTestWindowQuestionFlowRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> appended;
    std::uint32_t text_id{};

    std::uint32_t GameDB() override {
        calls.emplace_back("database");
        return 0x8000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void ResetQuestionCollection(pal4::PalTestWindowState&) override {
        calls.emplace_back("reset-collection");
    }
    void AppendQuestion(pal4::PalTestWindowState&,
                        std::uint32_t record) override {
        appended.push_back(record);
        calls.emplace_back("append");
    }
    void SetEnabled(std::uint32_t window, bool enabled) override {
        calls.emplace_back(std::string(enabled ? "enable:" : "disable:") +
                           std::to_string(window));
    }
    void SetSelected(std::uint32_t window, bool selected) override {
        Check(!selected, "reset clears answer selection");
        calls.emplace_back("unselect:" + std::to_string(window));
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        calls.emplace_back(std::string(visible ? "show:" : "hide:") +
                           std::to_string(window));
    }
    void UpdateQuestionAndAnswers(pal4::PalTestWindowState&) override {
        calls.emplace_back("update");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0xc000u;
    }
    std::uint32_t InsertActive(std::uint32_t array,
                               std::uint32_t member) override {
        Check(array == 0xc134u && member == 0x6084u,
              "question member inserted in active array");
        calls.emplace_back("insert");
        return 0x1111u;
    }
    std::uint32_t RemoveActive(std::uint32_t array,
                               std::uint32_t member) override {
        Check(array == 0xc134u && member == 0x6084u,
              "question member removed from active array");
        calls.emplace_back("remove");
        return 0x2222u;
    }
    void SetCompletionFlag(std::uint32_t offset,
                           std::uint32_t value) override {
        Check(value == 1u, "completed category stores one");
        calls.emplace_back("flag:" + std::to_string(offset));
    }
    void SaveSettings() override { calls.emplace_back("save"); }
    void PlayMusic(std::string_view name, bool loop) override {
        Check(name == "WA23.WAV" && !loop,
              "success plays verified WA23.WAV without loop");
        calls.emplace_back("music");
    }
    std::string_view TriggerItemText(std::uint32_t item) override {
        text_id = item;
        calls.emplace_back("text");
        return "result text";
    }
    std::uint32_t ShowCommonDialog(
        std::uint32_t manager, std::string_view text, float width,
        float height, std::uint32_t mode, std::uint32_t flags) override {
        Check(manager == 0xc000u && text == "result text" &&
                  width == 0.4f && height == 0.4f &&
                  mode == 4u && flags == 0u,
              "result dialog size, mode and text");
        calls.emplace_back("dialog");
        return 0xbeefu;
    }
};

pal4::PalTestWindowState MakeState() {
    pal4::PalTestWindowState state{};
    state.Write(0x4cu, 0x2100u);
    state.Write(0x50u, 0x2200u);
    state.Write(0x54u, 0x2300u);
    state.Write(0x58u, 0x2400u);
    state.Write(0x5cu, 0x2500u);
    state.Write(0x60u, 0x2600u);
    state.Write(0x64u, 0x2700u);
    state.Write(0x6cu, 0xb000u);
    return state;
}

void CheckQuestionSelection() {
    auto state = MakeState();
    Runtime runtime;
    runtime.words = {{0x80a0u, 0x9000u}, {0x9060u, 0xa000u},
                     {0x9064u, 0xa0e4u}, {0xa048u, 2u},
                     {0xa094u, 1u}, {0xa0e0u, 2u}};
    Check(pal4::PalTestWindowQuestionFlow::SetCurrentQuestion(
              state, 0x6000u, 1u, runtime) == 0x1111u &&
                  runtime.appended == std::vector<std::uint32_t>{
                      0xa000u, 0xa098u} &&
                  state.Read(0x70u) == 0xb000u &&
                  state.Read(0x78u) == 0u &&
                  state.bytes[0x7cu] == 1u &&
                  state.Read(0x3cu) == 1u &&
                  state.Read(0x80u) == 1u,
          "category filters 76-byte records and resets answer state");
    const auto& calls = runtime.calls;
    const auto find = [&](std::string_view value) {
        for (std::size_t index = 0u; index < calls.size(); ++index)
            if (calls[index] == value) return index;
        return calls.size();
    };
    Check(find("reset-collection") < find("append") &&
                  find("enable:8448") < find("disable:8704") &&
                  find("show:9984") < find("update") &&
                  find("update") < find("insert"),
          "selection respects collection and UI ordering");
    runtime.calls.clear();
    Check(pal4::PalTestWindowQuestionFlow::ResetUI(
              state, 0x6000u, runtime) == 0x2222u &&
                  runtime.calls == std::vector<std::string>{
                      "hide:9984", "enable:8448", "enable:8704",
                      "enable:8960", "unselect:9216", "unselect:9472",
                      "unselect:9728", "manager", "remove"},
          "reset hides panel, restores buttons and removes question member");
}

void CheckAnswerResults() {
    auto state = MakeState();
    Runtime runtime;
    state.bytes[0x7cu] = 1u;
    state.Write(0x80u, 2u);
    Check(pal4::PalTestWindowQuestionFlow::ProcessAnswer(
              state, 0x6000u, runtime) == 0xbeefu &&
                  runtime.text_id == 0xd7u &&
                  runtime.calls.front() == "flag:36" &&
                  runtime.calls[1] == "save" &&
                  runtime.calls.back() == "dialog",
          "successful category two persists and displays success");
    runtime.calls.clear();
    state.Write(0x80u, 3u);
    Check(pal4::PalTestWindowQuestionFlow::ProcessAnswer(
              state, 0x6000u, runtime) == 0xbeefu &&
                  runtime.calls.front() == "hide:9984" &&
                  runtime.text_id == 0xd7u,
          "unknown success category skips persistence");
    runtime.calls.clear();
    state.bytes[0x7cu] = 0u;
    Check(pal4::PalTestWindowQuestionFlow::ProcessAnswer(
              state, 0x6000u, runtime) == 0xbeefu &&
                  runtime.text_id == 0xd8u &&
                  runtime.calls.front() == "hide:9984" &&
                  runtime.calls.back() == "dialog",
          "failed answer displays failure without saving or music");
    for (const auto& call : runtime.calls)
        Check(call != "save" && call != "music" &&
                  call.rfind("flag:", 0u) != 0u,
              "failed answer has no success side effects");
}

}  // namespace

int main() {
    CheckQuestionSelection();
    CheckAnswerResults();
    std::cout << "Pal test window question flow tests passed\n";
}
