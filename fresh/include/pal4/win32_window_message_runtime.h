#pragma once

#include "pal4/game_window_lifecycle.h"
#include "pal4/window_message_processor.h"

#include <string>
#include <utility>

namespace pal4 {

// Supplies the imported Win32 calls used by the game WndProc. The injected
// service handles only game subsystem calls until those systems are composed.
class Win32WindowMessageRuntime final : public IWindowMessageRuntime {
public:
    Win32WindowMessageRuntime(GameWindowLifecycle& window,
                              IGameWindowRuntime& window_runtime,
                              IWindowMessageGameServices& game_services,
                              std::string target_display = {})
        : window_(window), window_runtime_(window_runtime),
          game_services_(game_services),
          target_display_(std::move(target_display)) {}

    void AcceptDroppedFiles(std::uint32_t window) override;
    void EnumCurrentDisplayMode(DisplayModeRecord& mode) override;
    void ChangeDisplayMode(const DisplayModeRecord& mode,
                           std::uint32_t flags) override;
    void ReleaseCursorClip() override;
    void ClipCursorToWindow(std::uint32_t window) override;
    void SetInputMode(std::uint32_t keyboard, std::uint32_t mouse,
                      std::uint32_t flags) override;
    void InitializeInputState() override;
    [[nodiscard]] std::pair<bool, bool> InputDevicesReady() override;
    void ShowWindow(std::uint32_t window, std::uint32_t command) override;
    void SetForegroundWindow(std::uint32_t window) override;
    void SetFocus(std::uint32_t window) override;
    void SetAudioActive(bool active) override;
    void Sleep(std::uint32_t milliseconds) override;
    [[nodiscard]] std::uint32_t TimeGetTime() override;
    void SetCapture(std::uint32_t window) override;
    void ReleaseCapture() override;
    void HandleKeyboardInput(std::uint32_t type,
                             KeyboardInputArgument argument) override;
    void HandleMouseInput(std::uint32_t type,
                          const std::uint32_t* data) override;
    void SetUICursorFromTable(std::uint32_t table_address) override;
    [[nodiscard]] bool CurrentAudioActive() override;
    void ReplayCurrentSound() override;
    [[nodiscard]] std::string ComposeMusicBasePath() override;
    [[nodiscard]] bool MusicEnabled() override;
    void SetMusicEnabled(bool enabled) override;
    [[nodiscard]] std::uint32_t PlayMusic(std::string_view path,
                                           std::uint32_t flags) override;
    void StoreMusicHandle(std::uint32_t handle) override;
    [[nodiscard]] ClientRect ReadSizingRect(std::uint32_t address) override;
    void SetWindowPos(std::uint32_t window, std::int32_t width,
                      std::int32_t height, std::uint32_t flags) override;
    void GetRenderSystem() override;
    [[nodiscard]] std::uint32_t GetActiveCamera() override;
    void ResizeRenderScreen(std::int32_t width, std::int32_t height,
                            std::uint32_t camera) override;
    [[nodiscard]] std::uint32_t GetFilesystemCallback() override;
    [[nodiscard]] std::uint32_t DroppedFileCount(
        std::uint32_t drop_handle) override;
    [[nodiscard]] std::string DroppedFile(std::uint32_t drop_handle,
                                          std::uint32_t index) override;
    [[nodiscard]] std::uint32_t CreateUncdFilesystemCallback(
        std::uint32_t id, std::string_view label) override;
    void RegisterFilesystemCallback(std::uint32_t callback) override;
    [[nodiscard]] std::uint32_t FindFilesystemCallback(
        std::string_view label) override;
    void SetFilesystemCallback(std::uint32_t callback) override;
    void FinishDrop(std::uint32_t drop_handle) override;
    [[nodiscard]] bool ConsoleActive() override;
    void HandleUIMessage(std::uint32_t window, std::uint32_t message,
                         std::uint32_t wparam, std::uint32_t lparam) override;
    [[nodiscard]] std::uint32_t DefWindowProc(
        std::uint32_t window, std::uint32_t message,
        std::uint32_t wparam, std::uint32_t lparam) override;

private:
    GameWindowLifecycle& window_;
    IGameWindowRuntime& window_runtime_;
    IWindowMessageGameServices& game_services_;
    std::string target_display_;
};

}  // namespace pal4
