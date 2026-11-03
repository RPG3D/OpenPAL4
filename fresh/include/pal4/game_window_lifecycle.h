#pragma once

#include "pal4/game_config_initializer.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct WindowRect {
    std::int32_t left = 0;
    std::int32_t top = 0;
    std::int32_t right = 0;
    std::int32_t bottom = 0;
};

struct WindowClassInfo {
    std::uint32_t size = 48;
    std::uint32_t style = 0x2000u;
    std::uint32_t window_procedure = 0;
    std::uint32_t instance = 0;
    std::uint32_t icon = 0;
    std::uint32_t cursor = 0;
    std::uint32_t small_icon = 0;
    std::string_view class_name;
};

struct WindowCreateInfo {
    std::string_view class_name;
    std::string_view window_name;
    std::uint32_t style = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::uint32_t instance = 0;
};

class WindowObjectState final {
public:
    static constexpr std::uint32_t kBytes = 0x118u;

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const;
    void Write(std::uint32_t offset, std::uint32_t value);
    void CopyClassName(std::string_view name);
    [[nodiscard]] std::string ClassName() const;
    [[nodiscard]] WindowRect ReadRect(std::uint32_t offset) const;
    void WriteRect(std::uint32_t offset, const WindowRect& rect);

private:
    std::array<std::uint8_t, kBytes> bytes_{};
};

class IGameWindowRuntime {
public:
    virtual ~IGameWindowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t LoadArrowCursor() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterClass(
        const WindowClassInfo& info) = 0;
    [[nodiscard]] virtual std::int32_t GetSystemMetric(std::uint32_t index) = 0;
    virtual void AdjustWindowRect(WindowRect& rect, std::uint32_t style) = 0;
    [[nodiscard]] virtual std::uint32_t CreateWindow(
        const WindowCreateInfo& info) = 0;
    [[nodiscard]] virtual std::uint32_t LoadMenu(
        std::uint32_t instance, std::string_view name) = 0;
    virtual void SetMenu(std::uint32_t window, std::uint32_t menu) = 0;
    virtual void ShowWindow(std::uint32_t window, std::uint32_t command) = 0;
    virtual void GetClientRect(std::uint32_t window, WindowRect& rect) = 0;
    virtual void MoveWindow(std::uint32_t window, std::int32_t width,
                            std::int32_t height, bool repaint) = 0;
    virtual void GetWindowRect(std::uint32_t window, WindowRect& rect) = 0;
    [[nodiscard]] virtual std::uint32_t ClipCursor(
        const WindowRect& rect) = 0;
    [[nodiscard]] virtual std::uint32_t TimeGetTime() = 0;
    virtual void SeedLegacyRandom(std::uint32_t seed) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeGameSystems() = 0;
    [[nodiscard]] virtual std::uint32_t InitializeDebugWindow() = 0;
    virtual void DestroyDebugWindow(std::uint32_t object) = 0;
    [[nodiscard]] virtual bool DestroyMenu(std::uint32_t menu) = 0;
    [[nodiscard]] virtual bool DestroyWindow(std::uint32_t window) = 0;
    [[nodiscard]] virtual bool UnregisterClass(std::string_view name,
                                                std::uint32_t instance) = 0;
    virtual void ShowShutdownError(std::uint32_t owner,
                                   std::string_view message,
                                   std::string_view caption,
                                   std::uint32_t flags) = 0;
    virtual void ShutdownGameSystems() = 0;
    virtual void EndTimePeriod() = 0;
    [[nodiscard]] virtual std::uint32_t Log(std::string_view format,
                                             std::string_view source_file,
                                             std::uint32_t source_line) = 0;
};

class GameWindowLifecycle final {
public:
    GameWindowLifecycle(WindowObjectState& window, GameRenderState& game,
                        GameConfigWords& config)
        : window_(window), game_(game), config_(config) {}

    [[nodiscard]] std::uint32_t Create(
        std::string_view class_name, std::string_view window_name,
        std::uint32_t window_procedure, bool bordered,
        const char* menu_name, std::uint32_t icon,
        std::uint32_t small_icon, IGameWindowRuntime& runtime);
    [[nodiscard]] std::uint32_t Destroy(IGameWindowRuntime& runtime);
    [[nodiscard]] std::uint32_t ClipCursorToGameWindow(
        std::uint32_t window, IGameWindowRuntime& runtime) const;

private:
    WindowObjectState& window_;
    GameRenderState& game_;
    GameConfigWords& config_;
};

}  // namespace pal4
