#pragma once

#include "pal4/native_display_target.h"

namespace pal4::test {

// Resolve the user's test screen by its current mode; Windows display numbers
// can change after reconnecting or reconfiguring monitors.
inline NativeDisplayTarget TestDisplay() {
    const auto target = SelectNativeDisplayTarget();
    const POINT point{target.bounds.left, target.bounds.top};
    MONITORINFO info{sizeof(info)};
    if (target.bounds.right - target.bounds.left != 1600 ||
        target.bounds.bottom - target.bounds.top != 900 ||
        !GetMonitorInfoA(MonitorFromPoint(point, MONITOR_DEFAULTTONULL), &info) ||
        (info.dwFlags & MONITORINFOF_PRIMARY) != 0u)
        throw std::runtime_error("PAL4 tests require the secondary 1600x900 screen");
    return target;
}

// Low-level SDK probes also create auxiliary windows. Keep their creation
// policy here so a literal desktop origin cannot put a probe on the main screen.
inline HWND CreateTestWindow(const char* class_name, const char* title,
                             DWORD style, int width, int height,
                             HWND parent, HMENU menu, HINSTANCE instance,
                             void* parameter) {
    const auto display = TestDisplay();
    const auto window = CreateWindowA(class_name, title, style,
                                      display.bounds.left, display.bounds.top,
                                      width, height, parent, menu, instance,
                                      parameter);
    if (!window) return nullptr;
    MONITORINFOEXA actual{sizeof(actual)};
    if (!GetMonitorInfoA(MonitorFromWindow(window, MONITOR_DEFAULTTONULL),
                         &actual) || display.device != actual.szDevice) {
        DestroyWindow(window);
        throw std::runtime_error("PAL4 probe opened on the wrong screen");
    }
    return window;
}

}  // namespace pal4::test
