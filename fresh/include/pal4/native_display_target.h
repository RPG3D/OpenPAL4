#pragma once

#include <windows.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace pal4 {

struct NativeDisplayTarget final {
    std::string device;
    RECT bounds{};
};

inline std::string DefaultNativeDisplayDevice() {
    // Re-resolve after display reconfiguration instead of retaining a device
    // number whose mode or primary status may have changed since startup.
    return [] {
        std::string secondary;
        std::string primary;
        for (DWORD index = 0u;; ++index) {
            DISPLAY_DEVICEA device{};
            device.cb = sizeof(device);
            if (!EnumDisplayDevicesA(nullptr, index, &device, 0u)) break;
            if ((device.StateFlags & DISPLAY_DEVICE_ACTIVE) == 0u) continue;
            DEVMODEA mode{};
            mode.dmSize = sizeof(mode);
            if (!EnumDisplaySettingsExA(device.DeviceName,
                                        ENUM_CURRENT_SETTINGS, &mode, 0u))
                continue;
            if ((device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0u) {
                primary = device.DeviceName;
                continue;
            }
            if (mode.dmPelsWidth == 1600u && mode.dmPelsHeight == 900u)
                return std::string(device.DeviceName);
            if (secondary.empty()) secondary = device.DeviceName;
        }
        if (!secondary.empty()) return secondary;
        if (!primary.empty()) return primary;
        throw std::runtime_error("PAL4 display is unavailable");
    }();
}

inline NativeDisplayTarget SelectNativeDisplayTarget(
    const std::string_view preferred_device = {}) {
    const std::string device_name = preferred_device.empty()
                                        ? DefaultNativeDisplayDevice()
                                        : std::string(preferred_device);
    DEVMODEA mode{};
    mode.dmSize = sizeof(mode);
    if (!EnumDisplaySettingsExA(device_name.c_str(),
                                ENUM_CURRENT_SETTINGS, &mode, 0u))
        throw std::runtime_error("PAL4 target display is unavailable");
    const auto left = mode.dmPosition.x;
    const auto top = mode.dmPosition.y;
    return {device_name,
            {left, top,
             left + static_cast<std::int32_t>(mode.dmPelsWidth),
             top + static_cast<std::int32_t>(mode.dmPelsHeight)}};
}

}  // namespace pal4
