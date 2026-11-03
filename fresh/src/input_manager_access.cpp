#include "pal4/input_manager_access.h"

#include <stdexcept>

namespace pal4 {

InputManager& InputManagerAccess::Get(IInputManagerAccessRuntime& runtime) {
    if ((guard_ & 1u) == 0u) {
        guard_ = static_cast<std::uint8_t>(guard_ | 1u);
        (void)manager_.Record().Initialize();
        runtime.RegisterExit(kCleanupAddress);
    }
    return manager_;
}

void InputManagerAccess::Cleanup(IInputRuntime& runtime) {
    manager_.CleanupDevices(runtime);
}

std::uint16_t InputManagerAccess::KeyCodeAt(const std::uint32_t index) {
    if (index < 4u) {
        constexpr std::uint16_t first[]{8u, 9u, 13u, 27u};
        return first[index];
    }
    if (index < 149u) {
        return static_cast<std::uint16_t>(index + 28u);
    }
    if (index < kKeyCodeCount) {
        return static_cast<std::uint16_t>(index + 107u);
    }
    throw std::out_of_range("input key code index");
}

std::uint32_t InputManagerAccess::TimeGetTime(IInputRuntime& runtime) {
    return runtime.TimeMilliseconds();
}

std::uint32_t InputManagerAccess::ProcessInputs(
    IInputRuntime& input_runtime,
    IInputManagerAccessRuntime& access_runtime) {
    (void)Get(access_runtime).UpdateInputDeviceState(input_runtime);
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index < kKeyCodeCount; ++index) {
        const std::uint16_t key = KeyCodeAt(index);
        const std::uint16_t state = Get(access_runtime).GetKeyState(key);
        const std::uint32_t pressed =
            state == 1u || state == 3u ? 1u : 0u;
        const std::uint32_t time = input_runtime.TimeMilliseconds();
        result = Get(access_runtime).UpdateKeyTimingInfo(time, index,
                                                         pressed);
    }
    return result;
}

}  // namespace pal4
