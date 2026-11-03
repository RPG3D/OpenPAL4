#include "pal4/main_window_commands.h"

#include <array>

namespace pal4 {
namespace {

std::uint8_t ShowSubwindow(std::string_view command,
                           IMainWindowCommandRuntime& runtime) {
    runtime.HideCurrentWindow(0u, 0u);
    MainWindowEventWords words{1u, 0u};
    const auto manager = runtime.FrameManager();
    runtime.ProcessEvent(manager + 0x104u, command, words);
    return 1u;
}

}  // namespace

std::uint8_t MainWindowCommands::DispatchPrimaryButton(
    const std::uint32_t button, IMainWindowCommandRuntime& runtime) {
    switch (button) {
    case 0u: return StartNewGame(runtime);
    case 1u: return ShowLoadGame(runtime);
    case 2u: return ExitButton(runtime);
    default: return 0u;
    }
}

std::uint8_t MainWindowCommands::ShowMain(
    IMainWindowCommandRuntime& runtime) {
    MainWindowEventWords words{1u, 0u};
    const auto manager = runtime.FrameManager();
    runtime.ProcessEvent(manager + 0x104u, "show_logo_ui", words);
    return 1u;
}

std::uint8_t MainWindowCommands::CloseMain(
    IMainWindowCommandRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    const auto frame = runtime.FindFrame(manager, "MainWindow");
    if (frame == 0u) return 1u;
    runtime.SetFrameVisibility(frame, 0u, 0u);

    MainWindowEventWords words{};
    constexpr std::array<std::string_view, 8> commands{
        "show_cast_ui", "show_introduction_ui", "view_picture",
        "show_load_ui", "show_movie_preview", "show_picture_preview",
        "show_test_ui", "show_ol_info_ui"};
    for (const auto command : commands) {
        const auto current_manager = runtime.FrameManager();
        runtime.ProcessEvent(current_manager + 0x104u, command, words);
    }
    return 1u;
}

std::uint8_t MainWindowCommands::StartNewGame(
    IMainWindowCommandRuntime& runtime) {
    MainWindowEventWords words{};
    const auto manager = runtime.FrameManager();
    runtime.ProcessEvent(manager + 0x104u, "close_main_ui", words);
    runtime.InitializeGame();
    return 1u;
}

std::uint8_t MainWindowCommands::ShowMoviePreview(
    IMainWindowCommandRuntime& runtime) {
    return ShowSubwindow("show_movie_preview", runtime);
}

std::uint8_t MainWindowCommands::ShowPicturePreview(
    IMainWindowCommandRuntime& runtime) {
    return ShowSubwindow("show_picture_preview", runtime);
}

std::uint8_t MainWindowCommands::ShowAboutGame(
    IMainWindowCommandRuntime& runtime) {
    return ShowSubwindow("show_introduction_ui", runtime);
}

std::uint8_t MainWindowCommands::ShowGameInfo(
    IMainWindowCommandRuntime& runtime) {
    return ShowSubwindow("show_cast_ui", runtime);
}

std::uint8_t MainWindowCommands::ExitButton(
    IMainWindowCommandRuntime& runtime) {
    return ProcessExit(runtime);
}

std::uint8_t MainWindowCommands::ProcessExit(
    IMainWindowCommandRuntime& runtime) {
    MainWindowEventWords words{};
    const auto manager = runtime.FrameManager();
    runtime.ProcessEvent(manager + 0x104u, "ui_exit_game", words);
    return 1u;
}

std::uint8_t MainWindowCommands::ShowLoadGame(
    IMainWindowCommandRuntime& runtime) {
    return ShowSubwindow("show_load_ui", runtime);
}

std::uint8_t MainWindowCommands::ShowQuest(
    IMainWindowCommandRuntime& runtime) {
    return ShowSubwindow("show_test_ui", runtime);
}

}  // namespace pal4
