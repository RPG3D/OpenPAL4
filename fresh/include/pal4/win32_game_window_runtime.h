#pragma once

#include "pal4/game_window_lifecycle.h"
#include "pal4/random_number_generator.h"

#include <string>
#include <utility>

namespace pal4 {

class IWin32GameWindowServices {
public:
    virtual ~IWin32GameWindowServices() = default;
    [[nodiscard]] virtual std::uint32_t InitializeGameSystems() = 0;
    [[nodiscard]] virtual std::uint32_t InitializeDebugWindow() = 0;
    virtual void DestroyDebugWindow(std::uint32_t object) = 0;
    virtual void ShutdownGameSystems() = 0;
    [[nodiscard]] virtual std::uint32_t Log(std::string_view format,
                                             std::string_view source_file,
                                             std::uint32_t line) = 0;
};

class Win32GameWindowRuntime final : public IGameWindowRuntime {
public:
    Win32GameWindowRuntime(GameRenderState& game,
                           RandomNumberGenerator& random,
                           IWin32GameWindowServices& services,
                           std::string target_display = {},
                           bool nonactivating_test_window = false)
        : game_(game), random_(random), services_(services),
          target_display_(std::move(target_display)),
          nonactivating_test_window_(nonactivating_test_window) {}

    [[nodiscard]] std::uint32_t LoadArrowCursor() override;
    [[nodiscard]] std::uint32_t RegisterClass(
        const WindowClassInfo& info) override;
    [[nodiscard]] std::int32_t GetSystemMetric(std::uint32_t index) override;
    void AdjustWindowRect(WindowRect& rect, std::uint32_t style) override;
    [[nodiscard]] std::uint32_t CreateWindow(
        const WindowCreateInfo& info) override;
    [[nodiscard]] std::uint32_t LoadMenu(std::uint32_t instance,
                                         std::string_view name) override;
    void SetMenu(std::uint32_t window, std::uint32_t menu) override;
    void ShowWindow(std::uint32_t window, std::uint32_t command) override;
    void GetClientRect(std::uint32_t window, WindowRect& rect) override;
    void MoveWindow(std::uint32_t window, std::int32_t width,
                    std::int32_t height, bool repaint) override;
    void GetWindowRect(std::uint32_t window, WindowRect& rect) override;
    [[nodiscard]] std::uint32_t ClipCursor(const WindowRect& rect) override;
    [[nodiscard]] std::uint32_t TimeGetTime() override;
    void SeedLegacyRandom(std::uint32_t seed) override;
    [[nodiscard]] std::uint32_t InitializeGameSystems() override;
    [[nodiscard]] std::uint32_t InitializeDebugWindow() override;
    void DestroyDebugWindow(std::uint32_t object) override;
    [[nodiscard]] bool DestroyMenu(std::uint32_t menu) override;
    [[nodiscard]] bool DestroyWindow(std::uint32_t window) override;
    [[nodiscard]] bool UnregisterClass(std::string_view name,
                                        std::uint32_t instance) override;
    void ShowShutdownError(std::uint32_t owner, std::string_view message,
                           std::string_view caption,
                           std::uint32_t flags) override;
    void ShutdownGameSystems() override;
    void EndTimePeriod() override;
    [[nodiscard]] std::uint32_t Log(std::string_view format,
                                     std::string_view source_file,
                                     std::uint32_t line) override;

private:
    GameRenderState& game_;
    RandomNumberGenerator& random_;
    IWin32GameWindowServices& services_;
    std::string target_display_;
    bool nonactivating_test_window_{};
};

}  // namespace pal4
