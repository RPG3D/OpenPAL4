#include "pal4/input_manager.h"

#include <algorithm>
#include <utility>

namespace pal4 {
namespace {

constexpr std::pair<std::uint32_t, std::uint32_t> kScanToKey[]{
    {1, 27}, {2, 49}, {3, 50}, {4, 51}, {5, 52}, {6, 53},
    {7, 54}, {8, 55}, {9, 56}, {10, 57}, {11, 48}, {12, 45},
    {13, 61}, {14, 8}, {15, 9}, {16, 113}, {17, 119},
    {18, 101}, {19, 114}, {20, 116}, {21, 121}, {22, 117},
    {23, 105}, {24, 111}, {25, 112}, {26, 91}, {27, 93},
    {28, 13}, {29, 130}, {30, 97}, {31, 115}, {32, 100},
    {33, 102}, {34, 103}, {35, 104}, {36, 106}, {37, 107},
    {38, 108}, {39, 59}, {40, 39}, {41, 96}, {42, 128},
    {43, 92}, {44, 122}, {45, 120}, {46, 99}, {47, 118},
    {48, 98}, {49, 110}, {50, 109}, {51, 44}, {52, 46},
    {53, 47}, {54, 129}, {55, 157}, {56, 132}, {57, 32},
    {58, 173}, {59, 138}, {60, 139}, {61, 140}, {62, 141},
    {63, 142}, {64, 143}, {65, 144}, {66, 145}, {67, 146},
    {68, 147}, {69, 172}, {70, 174}, {71, 169}, {72, 170},
    {73, 171}, {74, 158}, {75, 166}, {76, 167}, {77, 168},
    {78, 159}, {79, 163}, {80, 164}, {81, 165}, {82, 162},
    {83, 161}, {87, 148}, {88, 149}, {156, 160}, {157, 131},
    {181, 156}, {183, 175}, {184, 133}, {199, 152},
    {200, 136}, {201, 154}, {203, 134}, {205, 135},
    {207, 153}, {208, 137}, {209, 155}, {210, 150},
    {211, 151}};

}  // namespace

std::uint8_t InputManager::ReadByte(const std::uint32_t offset) const {
    if (offset < InputStateRecord::kBytes) {
        return record_.Bytes().at(offset);
    }
    return tail_.at(offset - InputStateRecord::kBytes);
}

void InputManager::WriteByte(const std::uint32_t offset,
                             const std::uint8_t value) {
    if (offset < InputStateRecord::kBytes) {
        record_.Bytes().at(offset) = value;
    } else {
        tail_.at(offset - InputStateRecord::kBytes) = value;
    }
}

std::uint32_t InputManager::ReadDword(const std::uint32_t offset) const {
    std::uint32_t value = 0u;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        value |= static_cast<std::uint32_t>(ReadByte(offset + index))
                 << (index * 8u);
    }
    return value;
}

void InputManager::WriteDword(const std::uint32_t offset,
                               const std::uint32_t value) {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        WriteByte(offset + index,
                  static_cast<std::uint8_t>(value >> (index * 8u)));
    }
}

std::uint32_t InputManager::InitializeDirectInput(
    const std::uint32_t instance, const std::uint32_t window,
    const std::uint32_t mouse_cooperative_mode, IInputRuntime& runtime) {
    std::uint32_t direct_input = ReadDword(0u);
    const std::int32_t creation = runtime.CreateDirectInput(
        instance, kDirectInputVersion, kInterfaceId, direct_input);
    WriteDword(0u, direct_input);
    if (creation != 0) {
        return 0u;
    }

    std::uint32_t success = 1u;
    std::uint32_t mouse = ReadDword(8u);
    const std::int32_t mouse_creation =
        runtime.CreateDevice(direct_input, kMouseId, mouse);
    WriteDword(8u, mouse);
    if (mouse_creation == 0) {
        runtime.SetDataFormat(mouse, kMouseFormat);
        runtime.SetCooperativeLevel(
            mouse, window, mouse_cooperative_mode ? 5u : 6u);
        (void)AcquireMouseDevice(runtime);
        if (ReadDword(0x186cu) == 0u) {
            success = 0u;
        }
    }

    std::uint32_t keyboard = ReadDword(4u);
    const std::int32_t keyboard_creation =
        runtime.CreateDevice(direct_input, kKeyboardId, keyboard);
    WriteDword(4u, keyboard);
    if (keyboard_creation == 0) {
        runtime.SetDataFormat(keyboard, kKeyboardFormat);
        runtime.SetCooperativeLevel(keyboard, window, 6u);
        (void)AcquireKeyboardDevice(runtime);
        if (ReadDword(0x1870u) == 0u) {
            success = 0u;
        }
        (void)InitializeKeyboardMappings();
        (void)InitializeInputMappings();
    }

    std::int32_t x = static_cast<std::int32_t>(ReadDword(0x30u));
    std::int32_t y = static_cast<std::int32_t>(ReadDword(0x34u));
    runtime.GetCursorPosition(x, y);
    WriteDword(0x30u, static_cast<std::uint32_t>(x));
    WriteDword(0x34u, static_cast<std::uint32_t>(y));
    return success;
}

std::int32_t InputManager::SetInputMode(
    const std::uint32_t mouse_enabled, const std::uint32_t keyboard_enabled,
    const std::uint32_t ignored, IInputRuntime& runtime) {
    (void)ignored;
    WriteDword(0x186cu, mouse_enabled);
    if (mouse_enabled != 0u) {
        (void)AcquireMouseDevice(runtime);
    } else {
        (void)ReleaseMouseDevice(runtime);
    }
    WriteDword(0x1870u, keyboard_enabled);
    return keyboard_enabled != 0u ? AcquireKeyboardDevice(runtime)
                                  : ReleaseKeyboardDevice(runtime);
}

std::uint32_t InputManager::InitializeState() {
    auto& bytes = record_.Bytes();
    std::fill(bytes.begin() + 0x68u, bytes.begin() + 0xf20u,
              std::uint8_t{0});
    tail_.fill(0u);
    return 0u;
}

std::uint32_t InputManager::SetInputParameters(
    const std::uint32_t first, const std::uint32_t second,
    const std::uint32_t third, const std::uint32_t fourth) {
    WriteDword(0x10u, first);
    WriteDword(0x14u, second);
    WriteDword(0x18u, third);
    WriteDword(0x1cu, fourth);
    return third;
}

std::uint16_t InputManager::GetKeyState(
    const std::uint32_t virtual_key) const {
    return ReadByte(0x68u + static_cast<std::uint16_t>(virtual_key));
}

std::uint32_t InputManager::GetInputStateValues(
    std::uint32_t& first, std::uint32_t& second) const {
    first = ReadDword(0x48u);
    second = ReadDword(0x4cu);
    return second;
}

std::int32_t InputManager::AcquireMouseDevice(IInputRuntime& runtime) {
    const std::uint32_t device = ReadDword(8u);
    if (device == 0u) {
        return 0;
    }
    WriteDword(0x186cu, 1u);
    std::int32_t result = runtime.AcquireDevice(device);
    for (std::uint32_t retry = 0u; result != 0 && retry < 50u; ++retry) {
        result = runtime.AcquireDevice(ReadDword(8u));
    }
    if (result != 0) {
        WriteDword(0x186cu, 0u);
    }
    return result;
}

std::int32_t InputManager::ReleaseMouseDevice(
    IInputRuntime& runtime, const std::int32_t incoming_result) {
    WriteDword(0x186cu, 0u);
    const std::uint32_t device = ReadDword(8u);
    return device != 0u ? runtime.UnacquireDevice(device) : incoming_result;
}

std::int32_t InputManager::AcquireKeyboardDevice(IInputRuntime& runtime) {
    const std::uint32_t device = ReadDword(4u);
    if (device == 0u) {
        return 0;
    }
    WriteDword(0x1870u, 1u);
    std::int32_t result = runtime.AcquireDevice(device);
    for (std::uint32_t retry = 0u; result != 0 && retry < 50u; ++retry) {
        result = runtime.AcquireDevice(ReadDword(4u));
    }
    if (result != 0) {
        WriteDword(0x1870u, 0u);
        return result;
    }
    return static_cast<std::int32_t>(InitializeState());
}

std::int32_t InputManager::ReleaseKeyboardDevice(IInputRuntime& runtime) {
    const std::int32_t result = static_cast<std::int32_t>(InitializeState());
    WriteDword(0x1870u, 0u);
    const std::uint32_t device = ReadDword(4u);
    return device != 0u ? runtime.UnacquireDevice(device) : result;
}

std::uint32_t InputManager::UpdateKeyOrButtonState(
    const std::uint32_t code, const std::uint32_t pressed,
    IInputRuntime& runtime) {
    const std::uint32_t state_offset = 0x68u + code;
    const std::uint8_t previous = ReadByte(state_offset);
    const std::uint32_t result = (pressed & 0xffffff00u) | previous;
    if (pressed != 0u) {
        if (previous < 2u) {
            WriteByte(state_offset, 2u);
            WriteDword(0x278u + code * 4u, runtime.TimeMilliseconds());
            const std::uint32_t snapshot = 0x6b0u + code * 8u;
            const std::uint32_t first = ReadDword(0x40u);
            WriteDword(snapshot, first);
            WriteDword(snapshot + 4u, ReadDword(0x44u));
            return first;
        }
        if (previous == 2u) {
            WriteByte(state_offset, 3u);
        }
    } else if (previous == 2u || previous == 3u) {
        WriteByte(state_offset, 1u);
    } else if (previous == 1u) {
        WriteByte(state_offset, 0u);
    }
    return result;
}

std::int32_t InputManager::UpdateInputDeviceState(IInputRuntime& runtime) {
    if (ReadDword(0x1870u) != 0u) {
        if (runtime.GetDeviceState(ReadDword(4u), 256u, *this,
                                   0x176u) != 0) {
            return AcquireKeyboardDevice(runtime);
        }
        for (std::uint32_t index = 0u; index < 256u; ++index) {
            const std::uint32_t code = ReadDword(0x1030u + index * 4u);
            if (code != 0u) {
                (void)UpdateKeyOrButtonState(
                    code, ReadByte(0x176u + index) >> 7u, runtime);
            }
        }
    }
    if (ReadDword(0x186cu) == 0u) {
        return 0;
    }
    if (runtime.GetDeviceState(ReadDword(8u), 20u, *this, 0x1430u) != 0) {
        return AcquireMouseDevice(runtime);
    }

    const std::uint32_t old_x = ReadDword(0x30u);
    const std::uint32_t old_y = ReadDword(0x34u);
    std::int32_t x = static_cast<std::int32_t>(old_x);
    std::int32_t y = static_cast<std::int32_t>(old_y);
    runtime.GetCursorPosition(x, y);
    WriteDword(0x30u, static_cast<std::uint32_t>(x));
    WriteDword(0x48u, static_cast<std::uint32_t>(x) - old_x);
    WriteDword(0x34u, static_cast<std::uint32_t>(y));
    WriteDword(0x4cu, static_cast<std::uint32_t>(y) - old_y);
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        result = UpdateKeyOrButtonState(
            256u + index, ReadByte(0x143cu + index) >> 7u, runtime);
    }
    return static_cast<std::int32_t>(result);
}

std::uint32_t InputManager::InitializeInputMappings() {
    for (std::uint32_t offset = 0xf20u; offset < 0x102eu; ++offset) {
        WriteByte(offset, 0u);
    }
    for (std::uint32_t index = 0u; index < 26u; ++index) {
        WriteByte(0xf81u + index,
                  static_cast<std::uint8_t>('A' + index));
    }
    constexpr std::pair<std::uint32_t, std::uint32_t> symbols[]{
        {0xf50u, ')'}, {0xf51u, '!'}, {0xf52u, '@'},
        {0xf53u, '#'}, {0xf54u, '$'}, {0xf55u, '%'},
        {0xf56u, '^'}, {0xf57u, '&'}, {0xf58u, '*'},
        {0xf59u, '('}, {0xf4du, '_'}, {0xf5du, '+'},
        {0xf7bu, '{'}, {0xf7du, '}'}, {0xf7cu, '|'},
        {0xf5bu, ':'}, {0xf47u, '"'}, {0xf4cu, '<'},
        {0xf4eu, '>'}, {0xf4fu, '?'}, {0xf80u, '~'}};
    for (const auto [offset, value] : symbols) {
        WriteByte(offset, static_cast<std::uint8_t>(value));
    }
    return 26u;
}

std::uint32_t InputManager::InitializeKeyboardMappings() {
    for (std::uint32_t index = 0u; index < 256u; ++index) {
        WriteDword(0x1030u + index * 4u, 0u);
    }
    for (const auto [scan, key] : kScanToKey) {
        WriteDword(0x1030u + scan * 4u, key);
    }
    for (std::uint32_t scan = 0u; scan < 256u; ++scan) {
        const std::uint32_t key = ReadDword(0x1030u + scan * 4u);
        WriteDword(0x1444u + key * 4u, scan);
    }
    WriteDword(0x1844u, 1u);
    WriteDword(0x1848u, 2u);
    WriteDword(0x184cu, 2u);
    WriteDword(0x1850u, 2u);
    return 2u;
}

std::uint32_t InputManager::UpdateKeyTimingInfo(
    const std::uint32_t time, const std::uint32_t index,
    const std::uint32_t pressed) {
    const std::uint32_t record = 0x1878u + index * 16u;
    if (pressed != 0u) {
        if (ReadDword(record + 4u) != 0u) {
            const std::uint32_t table_index = ReadDword(record + 8u);
            const std::uint32_t value =
                ReadDword(0x1844u + table_index * 4u);
            WriteDword(record + 12u, 0u);
            WriteDword(record, value);
            WriteDword(record + 8u, value);
        }
        const std::uint32_t count = ReadDword(record + 12u) + 1u;
        WriteDword(record + 4u, 0u);
        WriteDword(record + 12u, count);
        return count;
    }

    const std::uint32_t previous_time = ReadDword(record + 4u);
    WriteDword(record + 12u, 0u);
    if (previous_time == 0u) {
        WriteDword(record + 4u, time);
        return 0u;
    }
    WriteDword(record, 0u);
    if (time - previous_time > 100u) {
        WriteDword(record + 8u, 0u);
    }
    return 0u;
}

void InputManager::CleanupDevices(IInputRuntime& runtime) {
    const std::uint32_t direct_input = ReadDword(0u);
    if (direct_input == 0u) {
        return;
    }
    const std::uint32_t keyboard = ReadDword(4u);
    if (keyboard != 0u) {
        (void)ReleaseKeyboardDevice(runtime);
        (void)runtime.ReleaseDevice(keyboard);
        WriteDword(4u, 0u);
    }
    const std::uint32_t mouse = ReadDword(8u);
    if (mouse != 0u) {
        (void)ReleaseMouseDevice(runtime);
        (void)runtime.ReleaseDevice(mouse);
        WriteDword(8u, 0u);
    }
    (void)runtime.ReleaseDevice(direct_input);
    WriteDword(0u, 0u);
}

}  // namespace pal4
