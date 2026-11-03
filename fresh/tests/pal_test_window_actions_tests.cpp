#include "pal4/pal_test_window_actions.h"

#include <cstdlib>
#include <iostream>
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

struct Runtime final : pal4::PalTestWindowActionsRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t selected{1u};

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void SetCurrentQuestion(pal4::PalTestWindowState&,
                            std::uint32_t question) override {
        Check(question == 2u, "category index forwarded");
        calls.emplace_back("question");
    }
    std::uint32_t SelectedAnswerIndex(
        pal4::PalTestWindowState&) override {
        calls.emplace_back("selected");
        return selected;
    }
    void ProcessAnswer(pal4::PalTestWindowState&) override {
        calls.emplace_back("process");
    }
    void UpdateQuestionAndAnswers(pal4::PalTestWindowState&) override {
        calls.emplace_back("update");
    }
    void SetSelectedAnswer(pal4::PalTestWindowState&,
                           std::uint32_t answer) override {
        Check(answer == 2u, "answer index forwarded");
        calls.emplace_back("answer");
    }
};

void CheckClickHandlers() {
    pal4::PalTestWindowState state{};
    Runtime runtime;
    runtime.words = {{0x7008u, 0x8000u}, {0x8118u, 2u},
                     {0x701cu, 0u}};
    Check(pal4::PalTestWindowActions::OnCategoryClick(
              state, 0x7000u, runtime) == 1u &&
                  runtime.calls == std::vector<std::string>{
                      "read:28680", "read:33048", "question"},
          "category click reads event window index");
    runtime.calls.clear();
    Check(pal4::PalTestWindowActions::OnAnswerClick(
              state, 0x7000u, runtime) == 1u &&
                  runtime.calls == std::vector<std::string>{
                      "read:28700", "read:28680", "read:33048", "answer"},
          "primary button selects answer");
    runtime.calls.clear();
    runtime.words[0x701cu] = 1u;
    Check(pal4::PalTestWindowActions::OnAnswerClick(
              state, 0x7000u, runtime) == 1u &&
                  runtime.calls == std::vector<std::string>{"read:28700"},
          "other mouse buttons are ignored");
}

void CheckOK() {
    pal4::PalTestWindowState state{};
    Runtime runtime;
    Check(pal4::PalTestWindowActions::OnOK(state, runtime) == 1u &&
                  runtime.calls == std::vector<std::string>{"process"},
          "empty answer sequence processes immediately");
    state.Write(0x6cu, 0x9000u);
    state.Write(0x70u, 0x9008u);
    state.Write(0x78u, 0u);
    state.bytes[0x7cu] = 1u;
    runtime.words = {{0x9000u, 0xa000u}, {0xa044u, 2u},
                     {0x9004u, 0xa100u}, {0xa144u, 3u}};
    runtime.calls.clear();
    Check(pal4::PalTestWindowActions::OnOK(state, runtime) == 1u &&
                  state.Read(0x78u) == 1u && state.bytes[0x7cu] == 1u &&
                  runtime.calls.back() == "update",
          "correct answer advances and preserves success flag");
    runtime.calls.clear();
    Check(pal4::PalTestWindowActions::OnOK(state, runtime) == 1u &&
                  state.Read(0x78u) == 2u && state.bytes[0x7cu] == 0u &&
                  runtime.calls.back() == "update",
          "incorrect answer clears success flag and advances");
    runtime.calls.clear();
    Check(pal4::PalTestWindowActions::OnOK(state, runtime) == 1u &&
                  runtime.calls == std::vector<std::string>{"process"},
          "exhausted answer sequence processes result");
}

}  // namespace

int main() {
    CheckClickHandlers();
    CheckOK();
    std::cout << "Pal test window action tests passed\n";
}
