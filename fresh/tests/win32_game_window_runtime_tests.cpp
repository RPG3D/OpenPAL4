#include "pal4/win32_game_window_runtime.h"

#include <windows.h>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t create_messages = 0u;
std::uint32_t destroy_messages = 0u;

LRESULT CALLBACK TestWindowProcedure(
    HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_CREATE) ++create_messages;
    if (message == WM_DESTROY) ++destroy_messages;
    return DefWindowProcA(window, message, wparam, lparam);
}

struct Services final : pal4::IWin32GameWindowServices {
    std::uint32_t initializations = 0u;
    std::uint32_t shutdowns = 0u;
    std::uint32_t debug_destroys = 0u;
    std::uint32_t logs = 0u;
    std::uint32_t system_result = 1u;

    std::uint32_t InitializeGameSystems() override {
        ++initializations;
        return system_result;
    }
    std::uint32_t InitializeDebugWindow() override { return 0x1234u; }
    void DestroyDebugWindow(std::uint32_t object) override {
        Check(object == 0x1234u, "debug window service receives object");
        ++debug_destroys;
    }
    void ShutdownGameSystems() override { ++shutdowns; }
    std::uint32_t Log(std::string_view, std::string_view,
                      std::uint32_t) override {
        ++logs;
        return 1u;
    }
};

void TestRealWindowLifecycle() {
    create_messages = 0u;
    destroy_messages = 0u;
    pal4::WindowObjectState window;
    pal4::GameRenderState game{0x8e31e0u};
    pal4::GameConfigWords config{};
    pal4::RandomNumberGenerator random;
    Services services;
    pal4::Win32GameWindowRuntime runtime{game, random, services};
    pal4::GameWindowLifecycle lifecycle{window, game, config};
    const auto instance = GetModuleHandleA(nullptr);
    game.Write(4u, static_cast<std::uint32_t>(
                       reinterpret_cast<std::uintptr_t>(instance)));
    config[0] = 640u;
    config[1] = 480u;
    const auto procedure = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&TestWindowProcedure));
    Check(lifecycle.Create("PAL4-Native-Window-Test", "PAL4 Native Window",
                           procedure, true, nullptr, 0u, 0u, runtime) == 1u,
          "reconstructed lifecycle creates a real Win32 window");
    const auto handle = reinterpret_cast<HWND>(
        static_cast<std::uintptr_t>(game.Read(0u)));
    MONITORINFOEXA display{};
    display.cbSize = sizeof(display);
    RECT bounds{};
    Check(GetMonitorInfoA(MonitorFromWindow(handle, MONITOR_DEFAULTTONULL),
                          &display) != FALSE &&
              (display.dwFlags & MONITORINFOF_PRIMARY) == 0u &&
              display.rcMonitor.right - display.rcMonitor.left == 1600 &&
              display.rcMonitor.bottom - display.rcMonitor.top == 900 &&
              GetWindowRect(handle, &bounds) != FALSE &&
              bounds.left >= display.rcMonitor.left &&
              bounds.top >= display.rcMonitor.top &&
              bounds.right <= display.rcMonitor.right &&
              bounds.bottom <= display.rcMonitor.bottom,
          "default game window stays within secondary 1600x900 display");
    char title[64]{};
    (void)GetWindowTextA(handle, title, sizeof(title));
    const auto client = window.ReadRect(0xf8u);
    Check(IsWindow(handle) &&
              std::string_view(title) == "PAL4 Native Window" &&
              create_messages == 1u &&
              client.right > 0 && client.bottom > 0 &&
              game.Read(0x24u) == 1u &&
              random.Index() == 624u && services.initializations == 1u,
          "window handle, WndProc, client state and game gate are live");

    (void)lifecycle.Destroy(runtime);
    WNDCLASSEXA registered{};
    registered.cbSize = sizeof(registered);
    Check(!IsWindow(handle) && destroy_messages == 1u &&
              !GetClassInfoExA(instance, "PAL4-Native-Window-Test",
                               &registered) &&
              services.shutdowns == 1u && services.debug_destroys == 1u &&
              services.logs == 2u,
          "window and registered class are released after game shutdown");
}

}  // namespace

int main() {
    TestRealWindowLifecycle();
    return EXIT_SUCCESS;
}
