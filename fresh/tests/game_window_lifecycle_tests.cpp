#include "pal4/game_window_lifecycle.h"
#include "pal4/random_number_generator.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameWindowRuntime {
public:
    std::vector<std::string> events;
    std::uint32_t class_result = 1u;
    std::uint32_t window_result = 0x4567u;
    std::uint32_t systems_result = 1u;
    bool menu_destroyed = true;
    bool window_destroyed = true;
    bool class_unregistered = true;
    std::uint32_t last_style = 0;
    std::int32_t last_width = 0;
    std::int32_t last_height = 0;
    std::uint32_t last_line = 0;
    std::string last_format;
    std::string last_source;
    pal4::WindowRect clipped{};
    pal4::RandomNumberGenerator generator;

    std::uint32_t LoadArrowCursor() override {
        events.emplace_back("cursor");
        return 0x99u;
    }
    std::uint32_t RegisterClass(const pal4::WindowClassInfo& info) override {
        events.emplace_back("register");
        Check(info.size == 48u && info.style == 0x2000u &&
                  info.window_procedure == 0x1234u &&
                  info.instance == 0xabcu && info.icon == 0x11u &&
                  info.cursor == 0x99u && info.small_icon == 0x22u &&
                  info.class_name == "PalClass",
              "registered class fields");
        return class_result;
    }
    std::int32_t GetSystemMetric(const std::uint32_t index) override {
        events.emplace_back("metric-" + std::to_string(index));
        if (index == 32u) return 8;
        if (index == 33u) return 6;
        if (index == 15u) return 24;
        return 100;
    }
    void AdjustWindowRect(pal4::WindowRect& rect,
                          const std::uint32_t style) override {
        events.emplace_back("adjust");
        last_style = style;
        Check(rect.left == 0 && rect.top == 0 && rect.right == 800 &&
                  rect.bottom == 600,
              "unadjusted rectangle");
        rect.left = -4;
        rect.top = -20;
        rect.right = 804;
        rect.bottom = 604;
    }
    std::uint32_t CreateWindow(const pal4::WindowCreateInfo& info) override {
        events.emplace_back("create");
        Check(info.class_name == "PalClass" &&
                  info.window_name == "PAL4" && info.style == last_style &&
                  info.instance == 0xabcu,
              "window creation fields");
        last_width = info.width;
        last_height = info.height;
        return window_result;
    }
    std::uint32_t LoadMenu(const std::uint32_t instance,
                           const std::string_view name) override {
        events.emplace_back("load-menu");
        Check(instance == 0xabcu && name == "menu", "menu loading fields");
        return 0x89u;
    }
    void SetMenu(const std::uint32_t window,
                 const std::uint32_t menu) override {
        events.emplace_back("set-menu");
        Check(window == 0x4567u && menu == 0x89u, "menu assignment");
    }
    void ShowWindow(const std::uint32_t window,
                    const std::uint32_t command) override {
        events.emplace_back("show");
        Check(window == 0x4567u && command == 1u, "show window arguments");
    }
    void GetClientRect(const std::uint32_t window,
                       pal4::WindowRect& rect) override {
        events.emplace_back("client-rect");
        Check(window == 0x4567u, "client rect window");
        rect = {0, 0, 800, 600};
    }
    void MoveWindow(const std::uint32_t window,
                    const std::int32_t width, const std::int32_t height,
                    const bool repaint) override {
        events.emplace_back("move");
        Check(window == 0x4567u && repaint, "move window flags");
        Check(width == last_width && height == last_height,
              "move matches creation dimensions");
    }
    void GetWindowRect(const std::uint32_t window,
                       pal4::WindowRect& rect) override {
        events.emplace_back("window-rect");
        Check(window == 0x4567u, "window rect handle");
        rect = {100, 200, 900, 800};
    }
    std::uint32_t ClipCursor(const pal4::WindowRect& rect) override {
        events.emplace_back("clip");
        clipped = rect;
        return 0x42u;
    }
    std::uint32_t TimeGetTime() override {
        events.emplace_back("tick");
        return 0x12345678u;
    }
    void SeedLegacyRandom(const std::uint32_t seed) override {
        events.emplace_back("seed-random");
        Check(seed == 0x12345678u, "legacy RNG receives WinMM time");
        generator.Seed(seed);
    }
    std::uint32_t InitializeGameSystems() override {
        events.emplace_back("systems-init");
        return systems_result;
    }
    std::uint32_t InitializeDebugWindow() override {
        events.emplace_back("debug-init");
        return 0x567u;
    }
    void DestroyDebugWindow(const std::uint32_t object) override {
        events.emplace_back("debug-destroy");
        Check(object == 0x567u, "debug window object");
    }
    bool DestroyMenu(const std::uint32_t menu) override {
        events.emplace_back("destroy-menu");
        Check(menu == 0x89u, "destroy menu handle");
        return menu_destroyed;
    }
    bool DestroyWindow(const std::uint32_t window) override {
        events.emplace_back("destroy-window");
        Check(window == 0x4567u, "destroy window handle");
        return window_destroyed;
    }
    bool UnregisterClass(const std::string_view name,
                         const std::uint32_t instance) override {
        events.emplace_back("unregister");
        Check(name == "PalClass" && instance == 0xabcu,
              "class unregistration fields");
        return class_unregistered;
    }
    void ShowShutdownError(const std::uint32_t owner,
                           const std::string_view message,
                           const std::string_view caption,
                           const std::uint32_t flags) override {
        Check(owner == 0u && caption == "SHUTDOWN ERROR" && flags == 0x40u,
              "shutdown MessageBoxA owner, caption and icon flags");
        events.emplace_back(std::string(message));
    }
    void ShutdownGameSystems() override { events.emplace_back("systems-stop"); }
    void EndTimePeriod() override { events.emplace_back("timer-stop"); }
    std::uint32_t Log(const std::string_view format,
                      const std::string_view source_file,
                      const std::uint32_t line) override {
        events.emplace_back("log");
        last_format = format;
        last_source = source_file;
        last_line = line;
        return 0xfeedu;
    }
};

struct Fixture {
    pal4::WindowObjectState window;
    pal4::GameRenderState game{0x8e31e0u};
    pal4::GameConfigWords config{};
    pal4::GameWindowLifecycle lifecycle{window, game, config};
    Runtime runtime;

    Fixture() {
        game.Write(4u, 0xabcu);
        config[0] = 800u;
        config[1] = 600u;
    }
    std::uint32_t Create(const bool bordered = false,
                         const char* menu = nullptr) {
        return lifecycle.Create("PalClass", "PAL4", 0x1234u,
                                bordered, menu, 0x11u, 0x22u, runtime);
    }
};

void CheckEvents(const std::vector<std::string>& actual,
                 const std::vector<std::string>& expected,
                 const char* message) {
    Check(actual == expected, message);
}

void TestCreateSuccess() {
    Fixture f;
    Check(f.Create(true, "menu") == 1u, "create succeeds");
    Check(f.runtime.last_style == 0x80ca0000u &&
              f.runtime.last_width == 808 && f.runtime.last_height == 624,
          "bordered dimensions and style");
    Check(f.window.Read(0x68u) == 0xabcu &&
              f.window.Read(0x6cu) == 0x4567u &&
              f.window.Read(0x70u) == 0x89u &&
              f.window.Read(0x74u) == 1u &&
              f.game.Read(0u) == 0x4567u && f.game.Read(0x24u) == 1u,
          "create object and game state writes");
    const auto outer = f.window.ReadRect(0x108u);
    const auto client = f.window.ReadRect(0xf8u);
    Check(outer.left == -4 && outer.top == -20 && outer.right == 804 &&
              outer.bottom == 604 && client.right == 800 &&
              client.bottom == 600,
          "adjusted and client rectangles");
    Check(f.runtime.last_line == 0x9eu &&
              f.runtime.last_format ==
                  "FILE:  [%s]\nLINE:  [%d]\nINFO:  rwcWindows::createWindows success\n\n\n" &&
              f.runtime.last_source ==
                  R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Windows.cpp)",
          "create success log matches original bytes");
    Check(f.runtime.generator.Index() == 624u &&
              f.runtime.generator.StateAt(0u) == 0x12345678u &&
              f.runtime.generator.StateAt(1u) ==
                  69069u * 0x12345678u,
          "window creation seeds the legacy RNG from WinMM time");
    CheckEvents(f.runtime.events,
                {"cursor", "register", "metric-0", "metric-1", "adjust",
                 "create", "load-menu", "set-menu", "show", "client-rect",
                 "move", "tick", "seed-random", "log", "systems-init"},
                "create call order");
}

void TestCreateStylesAndFailures() {
    Fixture borderless;
    Check(borderless.Create() == 1u, "borderless create succeeds");
    Check(borderless.runtime.last_style == 0x800a0000u,
          "borderless style");
    Check(borderless.window.Read(0x70u) == 0u, "no menu is untouched");

    Fixture fullscreen;
    fullscreen.game.Write(8u, 1u);
    Check(fullscreen.Create(true) == 1u, "fullscreen create succeeds");
    Check(fullscreen.runtime.last_style == 0x90000000u,
          "game state overrides border flag");

    Fixture register_failure;
    register_failure.runtime.class_result = 0x10000u;
    Check(register_failure.Create() == 0u,
          "register tests low 16 bits");
    Check(register_failure.window.Read(0x68u) == 0xabcu &&
              register_failure.window.ClassName() == "PalClass" &&
              register_failure.window.Read(0x6cu) == 0u &&
              register_failure.runtime.last_line == 0x53u &&
              register_failure.runtime.last_format ==
                  "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[rwcWindows::createWindows failed <RegisterClassEx>]\n\n\n",
          "register failure writes and log");
    CheckEvents(register_failure.runtime.events,
                {"cursor", "register", "log"},
                "register failure stops immediately");

    Fixture window_failure;
    window_failure.runtime.window_result = 0u;
    Check(window_failure.Create() == 0u, "window failure result");
    Check(window_failure.window.Read(0x6cu) == 0u &&
              window_failure.game.Read(0u) == 0u &&
              window_failure.window.Read(0x74u) == 0u,
          "window failure state");
    CheckEvents(window_failure.runtime.events,
                {"cursor", "register", "metric-0", "metric-1", "adjust",
                 "create"}, "window failure stops before show");

    Fixture systems_failure;
    systems_failure.runtime.systems_result = 0u;
    Check(systems_failure.Create() == 0u, "systems failure result");
    Check(systems_failure.game.Read(0u) == 0x4567u &&
              systems_failure.game.Read(0x24u) == 0u &&
              systems_failure.window.Read(0x74u) == 1u,
          "systems failure retains created window");
}

void TestClassNameCopy() {
    pal4::WindowObjectState state;
    state.Write(0xf4u, 0x7a7a7a7au);
    state.CopyClassName(std::string(150u, 'X'));
    Check(state.ClassName().size() == 128u &&
              state.Read(0xf4u) == 0x7a585858u,
          "strncpy copies only 127 bytes and preserves byte 128");
    state.CopyClassName("A");
    Check(state.ClassName() == "A" && state.Read(0x7cu) == 0u,
          "short class name zero fills copied region");
}

void TestDestroy() {
    Fixture success;
    Check(success.Create(true, "menu") == 1u, "destroy setup");
    success.runtime.events.clear();
    Check(success.lifecycle.Destroy(success.runtime) == 0xfeedu,
          "destroy forwards final log result");
    Check(success.window.Read(0x68u) == 0xabcu &&
              success.window.Read(0x6cu) == 0x4567u &&
              success.window.Read(0x70u) == 0x89u,
          "successful destruction retains handle fields");
    Check(success.runtime.last_line == 0xcau &&
              success.runtime.last_format ==
                  "FILE:  [%s]\nLINE:  [%d]\nINFO:  rwcWindows::destroyWindows success\n\n\n" &&
              success.runtime.last_source ==
                  R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Windows.cpp)",
          "destroy success log matches original bytes");
    CheckEvents(success.runtime.events,
                {"debug-init", "debug-destroy", "destroy-menu",
                 "destroy-window", "unregister", "systems-stop",
                 "timer-stop", "log"}, "destroy call order");

    Fixture failure;
    Check(failure.Create(true, "menu") == 1u, "failure setup");
    failure.runtime.events.clear();
    failure.runtime.menu_destroyed = false;
    failure.runtime.window_destroyed = false;
    failure.runtime.class_unregistered = false;
    Check(failure.lifecycle.Destroy(failure.runtime) == 0xfeedu,
          "destroy continues after failures");
    Check(failure.window.Read(0x68u) == 0u &&
              failure.window.Read(0x6cu) == 0u &&
              failure.window.Read(0x70u) == 0u,
          "failed releases clear their fields");
    CheckEvents(failure.runtime.events,
                {"debug-init", "debug-destroy", "destroy-menu",
                 "Could Not Release hMenu.", "destroy-window",
                 "Could Not Release hWnd.", "unregister",
                 "Could Not Unregister Class.", "systems-stop", "timer-stop",
                 "log"}, "destroy failure call order");

    Fixture fullscreen;
    fullscreen.game.Write(8u, 1u);
    fullscreen.window.Write(0x68u, 0xabcu);
    fullscreen.window.CopyClassName("PalClass");
    fullscreen.runtime.events.clear();
    (void)fullscreen.lifecycle.Destroy(fullscreen.runtime);
    CheckEvents(fullscreen.runtime.events,
                {"unregister", "systems-stop", "timer-stop", "log"},
                "fullscreen skips debug and absent handles");
}

void TestClipCursor() {
    Fixture windowed;
    Check(windowed.lifecycle.ClipCursorToGameWindow(
              0x4567u, windowed.runtime) == 0x42u &&
              windowed.runtime.clipped.left == 108 &&
              windowed.runtime.clipped.top == 230 &&
              windowed.runtime.clipped.right == 892 &&
              windowed.runtime.clipped.bottom == 794,
          "windowed cursor rect excludes frame and caption");
    CheckEvents(windowed.runtime.events,
                {"window-rect", "metric-32", "metric-33", "metric-15",
                 "metric-32", "metric-33", "clip"},
                "windowed metric query order");

    Fixture fullscreen;
    fullscreen.game.Write(8u, 1u);
    Check(fullscreen.lifecycle.ClipCursorToGameWindow(
              0x4567u, fullscreen.runtime) == 0x42u &&
              fullscreen.runtime.clipped.left == 100 &&
              fullscreen.runtime.clipped.top == 200 &&
              fullscreen.runtime.clipped.right == 900 &&
              fullscreen.runtime.clipped.bottom == 800,
          "fullscreen uses unadjusted window rect");
    CheckEvents(fullscreen.runtime.events, {"window-rect", "clip"},
                "fullscreen skips all metrics");
}

}  // namespace

int main() {
    TestCreateSuccess();
    TestCreateStylesAndFailures();
    TestClassNameCopy();
    TestDestroy();
    TestClipCursor();
    return EXIT_SUCCESS;
}
