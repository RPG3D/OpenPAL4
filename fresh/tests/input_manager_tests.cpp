#include "pal4/input_manager.h"
#include "pal4/input_manager_access.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t HashRecordBytes(const pal4::InputManager& input,
                              const std::uint32_t start,
                              const std::uint32_t size) {
    std::uint32_t hash = 2166136261u;
    for (std::uint32_t offset = 0u; offset < size; ++offset) {
        hash = (hash ^ input.ReadByte(start + offset)) * 16777619u;
    }
    return hash;
}

class Runtime final : public pal4::IInputRuntime {
public:
    std::int32_t CreateDirectInput(const std::uint32_t instance,
                                   const std::uint32_t version,
                                   const std::uint32_t interface_id,
                                   std::uint32_t& object) override {
        events.emplace_back("direct");
        Check(instance == 0x1234u && version == 0x800u &&
                  interface_id == 0x86d018u,
              "DirectInput8Create parameters");
        object = 0x1000u;
        return direct_result;
    }

    std::int32_t CreateDevice(const std::uint32_t direct_input,
                              const std::uint32_t device_id,
                              std::uint32_t& device) override {
        Check(direct_input == 0x1000u, "device parent handle");
        if (device_id == pal4::InputManager::kMouseId) {
            events.emplace_back("create-mouse");
            device = 0x2000u;
            return mouse_creation;
        }
        Check(device_id == pal4::InputManager::kKeyboardId,
              "device GUID address");
        events.emplace_back("create-keyboard");
        device = 0x3000u;
        return keyboard_creation;
    }

    void SetDataFormat(const std::uint32_t device,
                       const std::uint32_t format_id) override {
        if (device == 0x2000u) {
            Check(format_id == pal4::InputManager::kMouseFormat,
                  "mouse format address");
            events.emplace_back("format-mouse");
        } else {
            Check(device == 0x3000u &&
                      format_id == pal4::InputManager::kKeyboardFormat,
                  "keyboard format address");
            events.emplace_back("format-keyboard");
        }
    }

    void SetCooperativeLevel(const std::uint32_t device,
                             const std::uint32_t window,
                             const std::uint32_t flags) override {
        Check(window == 0x5678u, "cooperative window argument");
        cooperative.emplace_back(device, flags);
        events.emplace_back(device == 0x2000u ? "coop-mouse" :
                                               "coop-keyboard");
    }

    std::int32_t AcquireDevice(const std::uint32_t device) override {
        if (device == 0x2000u) {
            events.emplace_back("acquire-mouse");
            return mouse_acquire_count++ < mouse_failures ? -11 : 0;
        }
        Check(device == 0x3000u, "acquired device handle");
        events.emplace_back("acquire-keyboard");
        return keyboard_acquire_count++ < keyboard_failures ? -22 : 0;
    }
    std::int32_t UnacquireDevice(const std::uint32_t device) override {
        Check(device == 0x2000u || device == 0x3000u,
              "unacquired device handle");
        events.emplace_back(device == 0x2000u ? "unacquire-mouse" :
                                               "unacquire-keyboard");
        return device == 0x2000u ? 33 : 44;
    }
    std::uint32_t ReleaseDevice(const std::uint32_t device) override {
        if (device == 0x1000u) {
            events.emplace_back("release-direct");
        } else if (device == 0x2000u) {
            events.emplace_back("release-mouse");
        } else {
            Check(device == 0x3000u, "released device handle");
            events.emplace_back("release-keyboard");
        }
        return 77u;
    }
    std::int32_t GetDeviceState(const std::uint32_t device,
                                const std::uint32_t size,
                                pal4::InputManager& manager,
                                const std::uint32_t destination) override {
        if (device == 0x3000u) {
            Check(size == 256u && destination == 0x176u,
                  "keyboard state request size and destination");
            events.emplace_back("state-keyboard");
            if (keyboard_state_result == 0) {
                for (std::uint32_t index = 0u; index < size; ++index) {
                    manager.WriteByte(destination + index,
                                      keyboard_data[index]);
                }
            }
            return keyboard_state_result;
        }
        Check(device == 0x2000u && size == 20u &&
                  destination == 0x1430u,
              "mouse state request size and destination");
        events.emplace_back("state-mouse");
        if (mouse_state_result == 0) {
            for (std::uint32_t index = 0u; index < size; ++index) {
                manager.WriteByte(destination + index, mouse_data[index]);
            }
        }
        return mouse_state_result;
    }
    std::uint32_t TimeMilliseconds() override {
        events.emplace_back("time");
        return 1234u + time_calls++;
    }
    void GetCursorPosition(std::int32_t& x, std::int32_t& y) override {
        events.emplace_back("cursor");
        x = -12;
        y = 34;
    }

    std::vector<std::string> events;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> cooperative;
    std::int32_t direct_result = 0;
    std::int32_t keyboard_creation = 0;
    std::int32_t mouse_creation = 0;
    std::uint32_t keyboard_failures = 0u;
    std::uint32_t mouse_failures = 0u;
    std::uint32_t keyboard_acquire_count = 0u;
    std::uint32_t mouse_acquire_count = 0u;
    std::int32_t keyboard_state_result = 0;
    std::int32_t mouse_state_result = 0;
    std::array<std::uint8_t, 256> keyboard_data{};
    std::array<std::uint8_t, 20> mouse_data{};
    std::uint32_t time_calls = 0u;
};

class AccessRuntime final : public pal4::IInputManagerAccessRuntime {
public:
    void RegisterExit(const std::uint32_t callback_address) override {
        Check(callback_address == pal4::InputManagerAccess::kCleanupAddress,
              "registered input cleanup callback address");
        ++registrations;
    }
    std::uint32_t registrations = 0u;
};

void CheckDirectInputBranches() {
    Runtime normal;
    pal4::InputManager input;
    Check(input.InitializeDirectInput(0x1234u, 0x5678u, 1u, normal) == 1u &&
              input.ReadDword(0u) == 0x1000u &&
              input.ReadDword(4u) == 0x3000u &&
              input.ReadDword(8u) == 0x2000u &&
              input.ReadDword(0x30u) == static_cast<std::uint32_t>(-12) &&
              input.ReadDword(0x34u) == 34u,
          "successful DirectInput handles, state and cursor");
    Check(normal.events == std::vector<std::string>{
              "direct", "create-mouse", "format-mouse",
              "coop-mouse", "acquire-mouse", "create-keyboard",
              "format-keyboard", "coop-keyboard", "acquire-keyboard",
              "cursor"} &&
              normal.cooperative ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x2000u, 5u}, {0x3000u, 6u}} &&
              input.ReadDword(0x1034u) == 27u &&
              input.ReadDword(0x1070u) == 113u &&
              input.ReadByte(0xf81u) == 'A',
          "device initialization order and cooperative modes");

    Runtime direct_failure;
    direct_failure.direct_result = -1;
    pal4::InputManager failed;
    failed.WriteDword(0x30u, 0x12345678u);
    failed.WriteDword(0x34u, 0x87654321u);
    Check(failed.InitializeDirectInput(0x1234u, 0x5678u, 0u,
                                       direct_failure) == 0u &&
              direct_failure.events == std::vector<std::string>{"direct"} &&
              failed.ReadDword(0x30u) == 0x12345678u &&
              failed.ReadDword(0x34u) == 0x87654321u,
          "DirectInput creation failure exits before devices and cursor");

    Runtime mouse_failure;
    mouse_failure.mouse_creation = -1;
    pal4::InputManager no_mouse;
    Check(no_mouse.InitializeDirectInput(0x1234u, 0x5678u, 0u,
                                          mouse_failure) == 1u &&
              mouse_failure.events.size() == 7u &&
              mouse_failure.events[1] == "create-mouse" &&
              mouse_failure.events[2] == "create-keyboard" &&
              mouse_failure.events.back() == "cursor",
          "mouse creation failure still initializes keyboard and cursor");

    Runtime keyboard_failure;
    keyboard_failure.keyboard_creation = -1;
    pal4::InputManager no_keyboard;
    Check(no_keyboard.InitializeDirectInput(0x1234u, 0x5678u, 0u,
                                             keyboard_failure) == 1u &&
              keyboard_failure.cooperative ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x2000u, 6u}} &&
              no_keyboard.ReadDword(0x1034u) == 0u &&
              keyboard_failure.events.back() == "cursor",
          "keyboard creation failure skips its setup; mouse mode zero uses six");

    Runtime mouse_inactive;
    mouse_inactive.mouse_failures = 51u;
    pal4::InputManager mouse_lost;
    Check(mouse_lost.InitializeDirectInput(0x1234u, 0x5678u, 1u,
                                            mouse_inactive) == 0u &&
              mouse_inactive.events.back() == "cursor" &&
              mouse_inactive.mouse_acquire_count == 51u &&
              mouse_lost.ReadDword(0x186cu) == 0u,
          "51 failed mouse acquire calls mark initialization unsuccessful");

    Runtime keyboard_inactive;
    keyboard_inactive.keyboard_failures = 51u;
    pal4::InputManager keyboard_lost;
    Check(keyboard_lost.InitializeDirectInput(0x1234u, 0x5678u, 1u,
                                               keyboard_inactive) == 0u &&
              keyboard_inactive.events.back() == "cursor" &&
              keyboard_inactive.keyboard_acquire_count == 51u &&
              keyboard_lost.ReadDword(0x1870u) == 0u,
          "51 failed keyboard acquire calls mark initialization unsuccessful");
}

void CheckModeAndRecordOperations() {
    Runtime runtime;
    pal4::InputManager input;
    pal4::InputManager missing_devices;
    Check(missing_devices.SetInputMode(7u, 8u, 0xabcdu, runtime) == 0 &&
              missing_devices.ReadDword(0x186cu) == 7u &&
              missing_devices.ReadDword(0x1870u) == 8u &&
              runtime.events.empty(),
          "requested modes remain stored when both device handles are absent");
    input.WriteDword(8u, 0x2000u);
    input.WriteDword(4u, 0x3000u);
    Check(input.SetInputMode(7u, 8u, 0xabcdu, runtime) == 0 &&
              input.ReadDword(0x186cu) == 1u &&
              input.ReadDword(0x1870u) == 1u &&
              runtime.events == std::vector<std::string>{
                  "acquire-mouse", "acquire-keyboard"},
          "mode enabling acquires both devices and returns keyboard result");
    runtime.events.clear();
    Check(input.SetInputMode(0u, 0u, 0xfeedu, runtime) == 44 &&
              input.ReadDword(0x186cu) == 0u &&
              input.ReadDword(0x1870u) == 0u &&
              runtime.events == std::vector<std::string>{
                  "unacquire-mouse", "unacquire-keyboard"},
          "mode disabling invokes reset paths and ignores third argument");

    for (std::uint32_t offset = 0u; offset < input.kBytes; ++offset) {
        input.WriteByte(offset, 0xa5u);
    }
    Check(input.InitializeState() == 0u,
          "state initializer returns zero");
    for (std::uint32_t offset = 0u; offset < input.kBytes; ++offset) {
        const std::uint8_t expected =
            (offset >= 0x68u && offset < 0xf20u) || offset >= 0x1878u
                ? 0u : 0xa5u;
        Check(input.ReadByte(offset) == expected,
              "state initializer exact zero ranges and preserved gaps");
    }

    Check(input.SetInputParameters(1u, 2u, 3u, 4u) == 3u &&
              input.ReadDword(0x10u) == 1u &&
              input.ReadDword(0x14u) == 2u &&
              input.ReadDword(0x18u) == 3u &&
              input.ReadDword(0x1cu) == 4u,
          "parameter setter stores four words and returns third");
    input.WriteDword(0x48u, 0x11223344u);
    input.WriteDword(0x4cu, 0x55667788u);
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
    Check(input.GetInputStateValues(first, second) == 0x55667788u &&
              first == 0x11223344u && second == 0x55667788u,
          "input state getter forwards two fields and second result");

    input.WriteByte(0x69u, 0x82u);
    Check(input.GetKeyState(1u) == 0x82u &&
              input.GetKeyState(0x10001u) == 0x82u,
          "key state zero extends byte after 16-bit key mask");
    bool out_of_record = false;
    try {
        (void)input.GetKeyState(0xffffu);
    } catch (const std::out_of_range&) {
        out_of_record = true;
    }
    Check(out_of_record, "out-of-record key read is an explicit boundary");
}

void CheckDeviceTransitions() {
    pal4::InputManager input;
    Runtime runtime;
    Check(input.AcquireMouseDevice(runtime) == 0 &&
              input.ReleaseMouseDevice(runtime, 0x1234) == 0x1234 &&
              input.AcquireKeyboardDevice(runtime) == 0 &&
              input.ReleaseKeyboardDevice(runtime) == 0 &&
              runtime.events.empty(),
          "missing handles skip COM calls; mouse preserves incoming EAX");

    input.WriteDword(8u, 0x2000u);
    runtime.mouse_failures = 50u;
    Check(input.AcquireMouseDevice(runtime) == 0 &&
              runtime.mouse_acquire_count == 51u &&
              input.ReadDword(0x186cu) == 1u,
          "mouse succeeds on final allowed retry");
    Check(input.ReleaseMouseDevice(runtime) == 33 &&
              input.ReadDword(0x186cu) == 0u &&
              runtime.events.back() == "unacquire-mouse",
          "mouse release clears status before unacquire");

    input.WriteDword(4u, 0x3000u);
    input.WriteByte(0x68u, 0x7fu);
    input.WriteByte(0x1878u, 0x7fu);
    runtime.keyboard_failures = 2u;
    Check(input.AcquireKeyboardDevice(runtime) == 0 &&
              runtime.keyboard_acquire_count == 3u &&
              input.ReadDword(0x1870u) == 1u &&
              input.ReadByte(0x68u) == 0u &&
              input.ReadByte(0x1878u) == 0u,
          "keyboard retry success clears both input state ranges");
    input.WriteByte(0x69u, 0x7fu);
    Check(input.ReleaseKeyboardDevice(runtime) == 44 &&
              input.ReadDword(0x1870u) == 0u &&
              input.ReadByte(0x69u) == 0u &&
              runtime.events.back() == "unacquire-keyboard",
          "keyboard release clears state before unacquire");

    pal4::InputManager exhausted;
    Runtime failed_runtime;
    exhausted.WriteDword(8u, 0x2000u);
    exhausted.WriteDword(4u, 0x3000u);
    exhausted.WriteByte(0x68u, 0x7fu);
    failed_runtime.mouse_failures = 51u;
    failed_runtime.keyboard_failures = 51u;
    Check(exhausted.AcquireMouseDevice(failed_runtime) == -11 &&
              failed_runtime.mouse_acquire_count == 51u &&
              exhausted.ReadDword(0x186cu) == 0u,
          "exhausted mouse acquire returns final COM failure");
    Check(exhausted.AcquireKeyboardDevice(failed_runtime) == -22 &&
              failed_runtime.keyboard_acquire_count == 51u &&
              exhausted.ReadDword(0x1870u) == 0u &&
              exhausted.ReadByte(0x68u) == 0x7fu,
          "exhausted keyboard acquire preserves state and returns COM failure");
}

void CheckKeyStateTransitions() {
    Runtime runtime;
    pal4::InputManager input;
    input.WriteDword(0x40u, 0x123456abu);
    input.WriteDword(0x44u, 0xabcdef01u);
    Check(input.UpdateKeyOrButtonState(7u, 1u, runtime) == 0x123456abu &&
              input.ReadByte(0x6fu) == 2u &&
              input.ReadDword(0x278u + 7u * 4u) == 1234u &&
              input.ReadDword(0x6b0u + 7u * 8u) == 0x123456abu &&
              input.ReadDword(0x6b4u + 7u * 8u) == 0xabcdef01u &&
              runtime.time_calls == 1u,
          "initial press records time and two snapshot words");
    Check(input.UpdateKeyOrButtonState(7u, 1u, runtime) == 2u &&
              input.ReadByte(0x6fu) == 3u &&
              input.UpdateKeyOrButtonState(7u, 1u, runtime) == 3u &&
              runtime.time_calls == 1u,
          "held press moves state two to three without new timestamp");
    Check(input.UpdateKeyOrButtonState(7u, 0u, runtime) == 3u &&
              input.ReadByte(0x6fu) == 1u &&
              input.UpdateKeyOrButtonState(7u, 0u, runtime) == 1u &&
              input.ReadByte(0x6fu) == 0u &&
              input.UpdateKeyOrButtonState(7u, 0u, runtime) == 0u,
          "release moves held state through one to zero");
    input.WriteByte(0x6fu, 1u);
    Check(input.UpdateKeyOrButtonState(7u, 1u, runtime) == 0x123456abu &&
              input.ReadByte(0x6fu) == 2u && runtime.time_calls == 2u,
          "new press from state one gets a fresh timestamp");
    input.WriteByte(0x6fu, 3u);
    Check(input.UpdateKeyOrButtonState(7u, 0x100u, runtime) == 0x103u &&
              input.ReadByte(0x6fu) == 3u && runtime.time_calls == 2u,
          "held press preserves incoming EAX upper bits");
}

void CheckFrameUpdate() {
    pal4::InputManager input;
    Runtime runtime;
    Check(input.UpdateInputDeviceState(runtime) == 0 &&
              runtime.events.empty(),
          "disabled devices skip both state reads");
    input.WriteDword(4u, 0x3000u);
    input.WriteDword(8u, 0x2000u);
    input.WriteDword(0x1870u, 1u);
    input.WriteDword(0x186cu, 1u);
    input.WriteDword(0x1030u, 10u);
    input.WriteDword(0x1034u, 11u);
    input.WriteByte(0x68u + 11u, 2u);
    input.WriteByte(0x68u + 257u, 3u);
    input.WriteDword(0x40u, 0x123456abu);
    input.WriteDword(0x44u, 0xabcdef01u);
    input.WriteDword(0x30u, 5u);
    input.WriteDword(0x34u, 6u);
    runtime.keyboard_data[0] = 0x80u;
    runtime.mouse_data[12] = 0x80u;
    runtime.mouse_data[14] = 0x80u;
    Check(input.UpdateInputDeviceState(runtime) == 0x123456ab &&
              input.ReadByte(0x68u + 10u) == 2u &&
              input.ReadByte(0x68u + 11u) == 1u &&
              input.ReadByte(0x68u + 256u) == 2u &&
              input.ReadByte(0x68u + 257u) == 1u &&
              input.ReadByte(0x68u + 258u) == 2u &&
              input.ReadDword(0x30u) == static_cast<std::uint32_t>(-12) &&
              input.ReadDword(0x34u) == 34u &&
              input.ReadDword(0x48u) == static_cast<std::uint32_t>(-17) &&
              input.ReadDword(0x4cu) == 28u &&
              runtime.time_calls == 3u,
          "frame update processes mapped keys, buttons and cursor delta");
    Check(runtime.events.front() == "state-keyboard" &&
              runtime.events[2] == "state-mouse" &&
              runtime.events[3] == "cursor" &&
              runtime.events.back() == "time",
          "frame read order is keyboard, mouse, cursor, button states");

    pal4::InputManager keyboard_failure;
    Runtime keyboard_runtime;
    keyboard_failure.WriteDword(4u, 0x3000u);
    keyboard_failure.WriteDword(0x1870u, 1u);
    keyboard_runtime.keyboard_state_result = -1;
    Check(keyboard_failure.UpdateInputDeviceState(keyboard_runtime) == 0 &&
              keyboard_runtime.events == std::vector<std::string>{
                  "state-keyboard", "acquire-keyboard"},
          "keyboard state failure reacquires keyboard and returns early");

    pal4::InputManager mouse_failure;
    Runtime mouse_runtime;
    mouse_failure.WriteDword(8u, 0x2000u);
    mouse_failure.WriteDword(0x186cu, 1u);
    mouse_runtime.mouse_state_result = -1;
    Check(mouse_failure.UpdateInputDeviceState(mouse_runtime) == 0 &&
              mouse_runtime.events == std::vector<std::string>{
                  "state-mouse", "acquire-mouse"},
          "mouse state failure reacquires mouse and skips cursor processing");

    pal4::InputManager exhausted_keyboard;
    Runtime exhausted_keyboard_runtime;
    exhausted_keyboard.WriteDword(4u, 0x3000u);
    exhausted_keyboard.WriteDword(8u, 0x2000u);
    exhausted_keyboard.WriteDword(0x1870u, 1u);
    exhausted_keyboard.WriteDword(0x186cu, 1u);
    exhausted_keyboard.WriteByte(0x68u, 0x7fu);
    exhausted_keyboard_runtime.keyboard_state_result = -1;
    exhausted_keyboard_runtime.keyboard_failures = 51u;
    Check(exhausted_keyboard.UpdateInputDeviceState(
              exhausted_keyboard_runtime) == -22 &&
              exhausted_keyboard_runtime.keyboard_acquire_count == 51u &&
              exhausted_keyboard_runtime.events.front() == "state-keyboard" &&
              exhausted_keyboard_runtime.events.back() == "acquire-keyboard" &&
              exhausted_keyboard.ReadDword(0x1870u) == 0u &&
              exhausted_keyboard.ReadByte(0x68u) == 0x7fu,
          "keyboard read failure forwards exhausted reacquire result and skips mouse");

    pal4::InputManager exhausted_mouse;
    Runtime exhausted_mouse_runtime;
    exhausted_mouse.WriteDword(8u, 0x2000u);
    exhausted_mouse.WriteDword(0x186cu, 1u);
    exhausted_mouse.WriteDword(0x30u, 55u);
    exhausted_mouse_runtime.mouse_state_result = -1;
    exhausted_mouse_runtime.mouse_failures = 51u;
    Check(exhausted_mouse.UpdateInputDeviceState(exhausted_mouse_runtime) ==
              -11 &&
              exhausted_mouse_runtime.mouse_acquire_count == 51u &&
              exhausted_mouse.ReadDword(0x186cu) == 0u &&
              exhausted_mouse.ReadDword(0x30u) == 55u &&
              exhausted_mouse_runtime.events.front() == "state-mouse" &&
              exhausted_mouse_runtime.events.back() == "acquire-mouse",
          "mouse read failure forwards exhausted reacquire result and skips cursor");
}

void CheckMappings() {
    pal4::InputManager input;
    for (std::uint32_t offset = 0xf20u; offset < 0x1854u; ++offset) {
        input.WriteByte(offset, 0xa5u);
    }
    Check(input.InitializeInputMappings() == 26u &&
              input.ReadByte(0xf20u) == 0u &&
              input.ReadByte(0xf4bu) == 0u &&
              input.ReadByte(0xf47u) == '"' &&
              input.ReadByte(0xf50u) == ')' &&
              input.ReadByte(0xf59u) == '(' &&
              input.ReadByte(0xf81u) == 'A' &&
              input.ReadByte(0xf9au) == 'Z' &&
              input.ReadByte(0xf80u) == '~' &&
              input.ReadByte(0x102du) == 0u &&
              input.ReadByte(0x102eu) == 0xa5u,
          "shifted character table exact boundaries and return value");
    Check(HashRecordBytes(input, 0xf20u, 0x10eu) == 0x741a2129u,
          "complete character mapping matches IDA instructions");

    Check(input.InitializeKeyboardMappings() == 2u &&
              input.ReadDword(0x1030u) == 0u &&
              input.ReadDword(0x1034u) == 27u &&
              input.ReadDword(0x1070u) == 113u &&
              input.ReadDword(0x117cu) == 161u &&
              input.ReadDword(0x1180u) == 0u &&
              input.ReadDword(0x137cu) == 151u &&
              input.ReadDword(0x1380u) == 0u &&
              input.ReadDword(0x1444u) == 255u &&
              input.ReadDword(0x1444u + 113u * 4u) == 16u &&
              input.ReadDword(0x1444u + 151u * 4u) == 211u &&
              input.ReadDword(0x1444u + 176u * 4u) == 0xa5a5a5a5u &&
              input.ReadDword(0x1844u) == 1u &&
              input.ReadDword(0x1848u) == 2u &&
              input.ReadDword(0x184cu) == 2u &&
              input.ReadDword(0x1850u) == 2u,
          "scan map, inverse writes and four trailing flags");
    Check(HashRecordBytes(input, 0x1030u, 0x400u) == 0xe0e32c63u,
          "all 256 scan-code DWORDs match IDA assignments");
}

void CheckKeyTiming() {
    pal4::InputManager input;
    (void)input.InitializeKeyboardMappings();
    const std::uint32_t record = 0x1878u + 3u * 16u;
    input.WriteDword(record + 8u, 2u);
    input.WriteDword(record + 12u, 7u);
    Check(input.UpdateKeyTimingInfo(100u, 3u, 0u) == 0u &&
              input.ReadDword(record + 4u) == 100u &&
              input.ReadDword(record + 8u) == 2u &&
              input.ReadDword(record + 12u) == 0u,
          "first unpressed update records time and resets counter");
    input.WriteDword(record, 55u);
    Check(input.UpdateKeyTimingInfo(200u, 3u, 0u) == 0u &&
              input.ReadDword(record) == 0u &&
              input.ReadDword(record + 8u) == 2u,
          "interval equal to 100 preserves timing-table index");
    Check(input.UpdateKeyTimingInfo(201u, 3u, 0u) == 0u &&
              input.ReadDword(record + 8u) == 0u,
          "interval above 100 clears timing-table index");
    input.WriteDword(record + 8u, 2u);
    Check(input.UpdateKeyTimingInfo(202u, 3u, 1u) == 1u &&
              input.ReadDword(record) == 2u &&
              input.ReadDword(record + 4u) == 0u &&
              input.ReadDword(record + 8u) == 2u &&
              input.ReadDword(record + 12u) == 1u &&
              input.UpdateKeyTimingInfo(203u, 3u, 1u) == 2u,
          "pressed update uses table value and increments count");

    const std::uint32_t wrapped = 0x1878u + 4u * 16u;
    input.WriteDword(wrapped + 4u, 0xfffffffeu);
    input.WriteDword(wrapped + 8u, 1u);
    Check(input.UpdateKeyTimingInfo(2u, 4u, 0u) == 0u &&
              input.ReadDword(wrapped + 8u) == 1u &&
              input.ReadDword(record + 12u) == 2u,
          "unsigned wrapped interval is four and other record stays intact");

    const std::uint32_t no_previous_time = 0x1878u + 5u * 16u;
    input.WriteDword(no_previous_time, 55u);
    input.WriteDword(no_previous_time + 8u, 1u);
    input.WriteDword(no_previous_time + 12u, 7u);
    Check(input.UpdateKeyTimingInfo(0xdeadbeefu, 5u, 1u) == 8u &&
              input.ReadDword(no_previous_time) == 55u &&
              input.ReadDword(no_previous_time + 4u) == 0u &&
              input.ReadDword(no_previous_time + 8u) == 1u &&
              input.ReadDword(no_previous_time + 12u) == 8u,
          "pressed record without prior time increments without remapping");
}

void CheckSingletonAndCleanup() {
    pal4::InputManager input;
    std::uint8_t guard = 0x80u;
    for (std::uint32_t offset = 0u;
         offset < pal4::InputStateRecord::kBytes; ++offset) {
        input.WriteByte(offset, 0xa5u);
    }
    input.WriteByte(0x1878u, 0xa5u);
    AccessRuntime access_runtime;
    pal4::InputManagerAccess access(input, guard);
    Check(&access.Get(access_runtime) == &input && guard == 0x81u &&
              access_runtime.registrations == 1u &&
              input.ReadDword(0u) == 0u &&
              input.ReadDword(0x0cu) == 0x3f800000u &&
              input.ReadDword(0x30u) == 0xa5a5a5a5u &&
              input.ReadByte(0x1878u) == 0xa5u &&
              pal4::InputManagerAccess::kManagerAddress == 0x8e1420u &&
              pal4::InputManagerAccess::kGuardAddress == 0x8e3d78u,
          "first access initializes prefix, preserves tail and registers exit");
    input.WriteDword(0x10u, 0x1234u);
    Check(&access.Get(access_runtime) == &input &&
              access_runtime.registrations == 1u &&
              input.ReadDword(0x10u) == 0x1234u,
          "later access leaves object and registration unchanged");

    Runtime runtime;
    input.WriteDword(4u, 0x3000u);
    input.WriteDword(8u, 0x2000u);
    access.Cleanup(runtime);
    Check(input.ReadDword(4u) == 0x3000u &&
              input.ReadDword(8u) == 0x2000u && runtime.events.empty(),
          "cleanup with no parent DirectInput handle skips both devices");
    input.WriteDword(0u, 0x1000u);
    input.WriteDword(0x186cu, 1u);
    input.WriteDword(0x1870u, 1u);
    input.WriteByte(0x68u, 3u);
    access.Cleanup(runtime);
    Check(runtime.events == std::vector<std::string>{
              "unacquire-keyboard", "release-keyboard",
              "unacquire-mouse", "release-mouse", "release-direct"} &&
              input.ReadDword(0u) == 0u && input.ReadDword(4u) == 0u &&
              input.ReadDword(8u) == 0u &&
              input.ReadDword(0x186cu) == 0u &&
              input.ReadDword(0x1870u) == 0u &&
              input.ReadByte(0x68u) == 0u && guard == 0x81u,
          "cleanup releases keyboard, mouse and parent in order");
}

void CheckProcessInputs() {
    Runtime clock;
    Check(pal4::InputManagerAccess::TimeGetTime(clock) == 1234u &&
              clock.time_calls == 1u,
          "timeGetTime thunk forwards to the clock boundary");
    std::uint32_t key_table_hash = 0x811c9dc5u;
    for (std::uint32_t index = 0u;
         index < pal4::InputManagerAccess::kKeyCodeCount; ++index) {
        const std::uint16_t key = pal4::InputManagerAccess::KeyCodeAt(index);
        for (const std::uint8_t byte : {
                 static_cast<std::uint8_t>(key),
                 static_cast<std::uint8_t>(key >> 8u)}) {
            key_table_hash = (key_table_hash ^ byte) * 0x01000193u;
        }
    }
    Check(key_table_hash == 0xac88f843u,
          "complete key-code table matches IDA's 326 bytes");
    Check(pal4::InputManagerAccess::kKeyCodeTableAddress == 0x8a1790u &&
              pal4::InputManagerAccess::KeyCodeAt(0u) == 8u &&
              pal4::InputManagerAccess::KeyCodeAt(3u) == 27u &&
              pal4::InputManagerAccess::KeyCodeAt(4u) == 32u &&
              pal4::InputManagerAccess::KeyCodeAt(148u) == 176u &&
              pal4::InputManagerAccess::KeyCodeAt(149u) == 256u &&
              pal4::InputManagerAccess::KeyCodeAt(162u) == 269u,
          "163 key codes match the IDA table boundaries");
    bool past_end = false;
    try {
        (void)pal4::InputManagerAccess::KeyCodeAt(163u);
    } catch (const std::out_of_range&) {
        past_end = true;
    }
    Check(past_end, "key-code table rejects an index past 162");

    pal4::InputManager input;
    std::uint8_t guard = 0u;
    pal4::InputManagerAccess access(input, guard);
    AccessRuntime access_runtime;
    Runtime runtime;
    (void)access.Get(access_runtime);
    input.WriteByte(0x68u + 8u, 1u);
    input.WriteByte(0x68u + 9u, 3u);
    input.WriteByte(0x68u + 269u, 3u);
    Check(access.ProcessInputs(runtime, access_runtime) == 1u &&
              runtime.time_calls == 163u &&
              input.ReadDword(0x1878u + 12u) == 1u &&
              input.ReadDword(0x1878u + 16u + 12u) == 1u &&
              input.ReadDword(0x1878u + 2u * 16u + 4u) == 1236u &&
              input.ReadDword(0x1878u + 162u * 16u + 12u) == 1u &&
              access_runtime.registrations == 1u,
          "frame loop samples 163 times and updates first, middle, last records");
    Check(access.ProcessInputs(runtime, access_runtime) == 2u &&
              runtime.time_calls == 326u &&
              input.ReadDword(0x1878u + 162u * 16u + 12u) == 2u &&
              access_runtime.registrations == 1u,
          "second frame increments held-key count without reinitializing");
}

}  // namespace

int main() {
    CheckDirectInputBranches();
    CheckModeAndRecordOperations();
    CheckDeviceTransitions();
    CheckKeyStateTransitions();
    CheckFrameUpdate();
    CheckMappings();
    CheckKeyTiming();
    CheckSingletonAndCleanup();
    CheckProcessInputs();
    std::cout << "input manager tests passed\n";
}
