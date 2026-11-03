#include "pal4/game_window_lifecycle.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::string_view kSourceFile =
    R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Windows.cpp)";
constexpr std::string_view kRegisterError =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[rwcWindows::createWindows failed <RegisterClassEx>]\n\n\n";
constexpr std::string_view kCreateSuccess =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  rwcWindows::createWindows success\n\n\n";
constexpr std::string_view kDestroySuccess =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  rwcWindows::destroyWindows success\n\n\n";
constexpr std::string_view kShutdownCaption = "SHUTDOWN ERROR";
constexpr std::uint32_t kShutdownIcon = 0x40u;

}  // namespace

std::uint32_t WindowObjectState::Read(const std::uint32_t offset) const {
    if (offset > kBytes - sizeof(std::uint32_t)) {
        throw std::out_of_range("window field offset");
    }
    std::uint32_t value = 0;
    std::memcpy(&value, bytes_.data() + offset, sizeof(value));
    return value;
}

void WindowObjectState::Write(const std::uint32_t offset,
                              const std::uint32_t value) {
    if (offset > kBytes - sizeof(value)) {
        throw std::out_of_range("window field offset");
    }
    std::memcpy(bytes_.data() + offset, &value, sizeof(value));
}

void WindowObjectState::CopyClassName(const std::string_view name) {
    const auto length = std::min(name.size(), std::size_t{127});
    if (length != 0u) {
        std::memcpy(bytes_.data() + 0x78u, name.data(), length);
    }
    if (length < 127u) {
        std::memset(bytes_.data() + 0x78u + length, 0, 127u - length);
    }
}

std::string WindowObjectState::ClassName() const {
    const auto* begin = reinterpret_cast<const char*>(bytes_.data() + 0x78u);
    const auto* end = begin + 128;
    return {begin, std::find(begin, end, '\0')};
}

WindowRect WindowObjectState::ReadRect(const std::uint32_t offset) const {
    return {static_cast<std::int32_t>(Read(offset)),
            static_cast<std::int32_t>(Read(offset + 4u)),
            static_cast<std::int32_t>(Read(offset + 8u)),
            static_cast<std::int32_t>(Read(offset + 12u))};
}

void WindowObjectState::WriteRect(const std::uint32_t offset,
                                  const WindowRect& rect) {
    Write(offset, static_cast<std::uint32_t>(rect.left));
    Write(offset + 4u, static_cast<std::uint32_t>(rect.top));
    Write(offset + 8u, static_cast<std::uint32_t>(rect.right));
    Write(offset + 12u, static_cast<std::uint32_t>(rect.bottom));
}

std::uint32_t GameWindowLifecycle::Create(
    const std::string_view class_name, const std::string_view window_name,
    const std::uint32_t window_procedure, const bool bordered,
    const char* menu_name, const std::uint32_t icon,
    const std::uint32_t small_icon, IGameWindowRuntime& runtime) {
    window_.Write(0x68u, game_.Read(4u));
    window_.CopyClassName(class_name);
    const WindowClassInfo window_class{
        48u, 0x2000u, window_procedure, window_.Read(0x68u),
        icon, runtime.LoadArrowCursor(), small_icon, class_name};
    if ((runtime.RegisterClass(window_class) & 0xffffu) == 0u) {
        (void)runtime.Log(kRegisterError, kSourceFile, 0x53u);
        return 0u;
    }

    WindowRect outer{0, 0, static_cast<std::int32_t>(config_[0]),
                     static_cast<std::int32_t>(config_[1])};
    window_.WriteRect(0x108u, outer);
    (void)runtime.GetSystemMetric(0u);
    (void)runtime.GetSystemMetric(1u);
    const auto style = game_.Read(8u) != 0u
                           ? 0x90000000u
                           : (bordered ? 0x80ca0000u : 0x800a0000u);
    runtime.AdjustWindowRect(outer, style);
    window_.WriteRect(0x108u, outer);
    const auto width = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(outer.right) -
        static_cast<std::uint32_t>(outer.left));
    const auto height = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(outer.bottom) -
        static_cast<std::uint32_t>(outer.top));
    const auto handle = runtime.CreateWindow(
        {class_name, window_name, style, width, height, window_.Read(0x68u)});
    window_.Write(0x6cu, handle);
    if (handle == 0u) {
        return 0u;
    }

    game_.Write(0u, handle);
    if (menu_name != nullptr) {
        const auto menu = runtime.LoadMenu(window_.Read(0x68u), menu_name);
        window_.Write(0x70u, menu);
        runtime.SetMenu(handle, menu);
    }
    runtime.ShowWindow(handle, 1u);
    WindowRect client = window_.ReadRect(0xf8u);
    runtime.GetClientRect(handle, client);
    window_.WriteRect(0xf8u, client);
    runtime.MoveWindow(handle, width, height, true);
    window_.Write(0x74u, 1u);
    runtime.SeedLegacyRandom(runtime.TimeGetTime());
    (void)runtime.Log(kCreateSuccess, kSourceFile, 0x9eu);
    if (runtime.InitializeGameSystems() == 0u) {
        return 0u;
    }
    game_.Write(0x24u, 1u);
    return 1u;
}

std::uint32_t GameWindowLifecycle::Destroy(IGameWindowRuntime& runtime) {
    if (game_.Read(8u) == 0u) {
        runtime.DestroyDebugWindow(runtime.InitializeDebugWindow());
    }
    const auto menu = window_.Read(0x70u);
    if (menu != 0u && !runtime.DestroyMenu(menu)) {
        runtime.ShowShutdownError(0u, "Could Not Release hMenu.",
                                  kShutdownCaption, kShutdownIcon);
        window_.Write(0x70u, 0u);
    }
    const auto handle = window_.Read(0x6cu);
    if (handle != 0u && !runtime.DestroyWindow(handle)) {
        runtime.ShowShutdownError(0u, "Could Not Release hWnd.",
                                  kShutdownCaption, kShutdownIcon);
        window_.Write(0x6cu, 0u);
    }
    if (!runtime.UnregisterClass(window_.ClassName(), window_.Read(0x68u))) {
        runtime.ShowShutdownError(0u, "Could Not Unregister Class.",
                                  kShutdownCaption, kShutdownIcon);
        window_.Write(0x68u, 0u);
    }
    runtime.ShutdownGameSystems();
    runtime.EndTimePeriod();
    return runtime.Log(kDestroySuccess, kSourceFile, 0xcau);
}

std::uint32_t GameWindowLifecycle::ClipCursorToGameWindow(
    const std::uint32_t window, IGameWindowRuntime& runtime) const {
    WindowRect rect;
    runtime.GetWindowRect(window, rect);
    if (game_.Read(8u) == 0u) {
        rect.left = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(rect.left) +
            static_cast<std::uint32_t>(runtime.GetSystemMetric(32u)));
        const auto frame = runtime.GetSystemMetric(33u);
        const auto caption = runtime.GetSystemMetric(15u);
        rect.top = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(rect.top) +
            static_cast<std::uint32_t>(frame) +
            static_cast<std::uint32_t>(caption));
        rect.right = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(rect.right) -
            static_cast<std::uint32_t>(runtime.GetSystemMetric(32u)));
        rect.bottom = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(rect.bottom) -
            static_cast<std::uint32_t>(runtime.GetSystemMetric(33u)));
    }
    return runtime.ClipCursor(rect);
}

}  // namespace pal4
