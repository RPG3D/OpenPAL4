#include "pal4/main_window_commands.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MainWindowCommandRuntimeStub {
    std::uint32_t frame = 0x6000u;
    std::uint32_t manager_calls{};
    std::uint32_t expected_first{};
    std::vector<std::string> calls;
    std::vector<std::string> commands;
    pal4::MainWindowEventWords* first_words{};

    std::uint32_t FrameManager() override {
        ++manager_calls;
        calls.emplace_back("manager");
        return 0x7000u;
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x7000u && name == "MainWindow",
              "close command finds MainWindow by its registered name");
        calls.emplace_back("find-frame");
        return frame;
    }
    void SetFrameVisibility(std::uint32_t target,
                            std::uint32_t showing,
                            std::uint32_t parent) override {
        Check(target == frame && showing == 0u && parent == 0u,
              "close command hides MainWindow with two zero arguments");
        calls.emplace_back("hide-frame");
    }
    void ProcessEvent(std::uint32_t tree, std::string_view command,
                      pal4::MainWindowEventWords& words) override {
        Check(tree == 0x7104u, "commands use manager tree +0x104");
        if (first_words == nullptr) first_words = &words;
        Check(first_words == &words,
              "all close commands reuse the same event words");
        Check(words.first == expected_first && words.second == 0u,
              "command receives expected two-word event arguments");
        commands.emplace_back(command);
        calls.emplace_back("process");
    }
    void InitializeGame() override {
        calls.emplace_back("initialize-game");
    }
    void HideCurrentWindow(std::uint32_t showing,
                           std::uint32_t parent) override {
        Check(showing == 0u && parent == 0u,
              "menu button hides current window with two zeros");
        calls.emplace_back("hide-current");
    }
};

}  // namespace

int main() {
    Runtime runtime;
    runtime.expected_first = 1u;
    Check(pal4::MainWindowCommands::ShowMain(runtime) == 1u &&
              runtime.manager_calls == 1u &&
              runtime.commands ==
                  (std::vector<std::string>{"show_logo_ui"}) &&
              runtime.calls ==
                  (std::vector<std::string>{"manager", "process"}),
          "show-main callback dispatches logo command");
    runtime = Runtime{};
    Check(pal4::MainWindowCommands::CloseMain(runtime) == 1u &&
              runtime.manager_calls == 9u &&
              runtime.calls[0] == "manager" &&
              runtime.calls[1] == "find-frame" &&
              runtime.calls[2] == "hide-frame" &&
              runtime.commands == (std::vector<std::string>{
                  "show_cast_ui", "show_introduction_ui", "view_picture",
                  "show_load_ui", "show_movie_preview",
                  "show_picture_preview", "show_test_ui",
                  "show_ol_info_ui"}) &&
              runtime.calls.back() == "process",
          "close-main hides frame then dispatches eight commands in order");
    runtime = Runtime{};
    runtime.frame = 0u;
    Check(pal4::MainWindowCommands::CloseMain(runtime) == 1u &&
              runtime.manager_calls == 1u &&
              runtime.calls ==
                  (std::vector<std::string>{"manager", "find-frame"}),
          "missing MainWindow suppresses all close commands");
    runtime = Runtime{};
    Check(pal4::MainWindowCommands::StartNewGame(runtime) == 1u &&
              runtime.commands ==
                  (std::vector<std::string>{"close_main_ui"}) &&
              runtime.calls ==
                  (std::vector<std::string>{
                      "manager", "process", "initialize-game"}),
          "new game closes main UI before initializing PALGameIV");
    using Handler = std::uint8_t (*)(pal4::IMainWindowCommandRuntime&);
    const std::array<std::pair<Handler, std::string_view>, 6> buttons{{
        {&pal4::MainWindowCommands::ShowMoviePreview,
         "show_movie_preview"},
        {&pal4::MainWindowCommands::ShowPicturePreview,
         "show_picture_preview"},
        {&pal4::MainWindowCommands::ShowAboutGame,
         "show_introduction_ui"},
        {&pal4::MainWindowCommands::ShowGameInfo,
         "show_cast_ui"},
        {&pal4::MainWindowCommands::ShowLoadGame,
         "show_load_ui"},
        {&pal4::MainWindowCommands::ShowQuest,
         "show_test_ui"}}};
    for (const auto& [handler, command] : buttons) {
        runtime = Runtime{};
        runtime.expected_first = 1u;
        Check(handler(runtime) == 1u &&
                  runtime.commands ==
                      (std::vector<std::string>{std::string{command}}) &&
                  runtime.calls ==
                      (std::vector<std::string>{
                          "hide-current", "manager", "process"}),
              "submenu buttons hide MainWindow before dispatching command");
    }
    runtime = Runtime{};
    Check(pal4::MainWindowCommands::ProcessExit(runtime) == 1u &&
              runtime.commands ==
                  (std::vector<std::string>{"ui_exit_game"}) &&
              runtime.calls ==
                  (std::vector<std::string>{"manager", "process"}),
          "exit processing dispatches ui_exit_game with zero words");
    runtime = Runtime{};
    Check(pal4::MainWindowCommands::ExitButton(runtime) == 1u &&
              runtime.commands ==
                  (std::vector<std::string>{"ui_exit_game"}),
          "exit button forwards to exit event processing");
    std::cout << "MainWindow command tests passed\n";
}
