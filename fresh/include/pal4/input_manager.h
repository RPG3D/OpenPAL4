#pragma once

#include <array>
#include <cstdint>

#include "pal4/input_state_record.h"

namespace pal4 {

class InputManager;

class IInputRuntime {
public:
    virtual ~IInputRuntime() = default;
    virtual std::int32_t CreateDirectInput(std::uint32_t instance,
                                           std::uint32_t version,
                                           std::uint32_t interface_id,
                                           std::uint32_t& object) = 0;
    virtual std::int32_t CreateDevice(std::uint32_t direct_input,
                                      std::uint32_t device_id,
                                      std::uint32_t& device) = 0;
    virtual void SetDataFormat(std::uint32_t device,
                               std::uint32_t format_id) = 0;
    virtual void SetCooperativeLevel(std::uint32_t device,
                                     std::uint32_t window,
                                     std::uint32_t flags) = 0;
    virtual std::int32_t AcquireDevice(std::uint32_t device) = 0;
    virtual std::int32_t UnacquireDevice(std::uint32_t device) = 0;
    virtual std::uint32_t ReleaseDevice(std::uint32_t device) = 0;
    virtual std::int32_t GetDeviceState(std::uint32_t device,
                                        std::uint32_t size,
                                        InputManager& manager,
                                        std::uint32_t destination) = 0;
    virtual std::uint32_t TimeMilliseconds() = 0;
    virtual void GetCursorPosition(std::int32_t& x, std::int32_t& y) = 0;
};

class InputManager final {
public:
    static constexpr std::uint32_t kBytes = 0x2958u;
    static constexpr std::uint32_t kDirectInputVersion = 0x800u;
    static constexpr std::uint32_t kInterfaceId = 0x86d018u;
    static constexpr std::uint32_t kMouseId = 0x86cfc8u;
    static constexpr std::uint32_t kKeyboardId = 0x86cfb8u;
    static constexpr std::uint32_t kMouseFormat = 0x86d41cu;
    static constexpr std::uint32_t kKeyboardFormat = 0x86d214u;

    [[nodiscard]] InputStateRecord& Record() { return record_; }
    [[nodiscard]] const InputStateRecord& Record() const { return record_; }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t offset) const;
    void WriteByte(std::uint32_t offset, std::uint8_t value);
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const;
    void WriteDword(std::uint32_t offset, std::uint32_t value);

    [[nodiscard]] std::uint32_t InitializeDirectInput(
        std::uint32_t instance, std::uint32_t window,
        std::uint32_t mouse_cooperative_mode, IInputRuntime& runtime);
    [[nodiscard]] std::int32_t SetInputMode(
        std::uint32_t mouse_enabled, std::uint32_t keyboard_enabled,
        std::uint32_t ignored, IInputRuntime& runtime);
    [[nodiscard]] std::uint32_t InitializeState();
    [[nodiscard]] std::uint32_t SetInputParameters(
        std::uint32_t first, std::uint32_t second, std::uint32_t third,
        std::uint32_t fourth);
    [[nodiscard]] std::uint16_t GetKeyState(std::uint32_t virtual_key) const;
    [[nodiscard]] std::uint32_t GetInputStateValues(
        std::uint32_t& first, std::uint32_t& second) const;
    [[nodiscard]] std::int32_t AcquireMouseDevice(IInputRuntime& runtime);
    [[nodiscard]] std::int32_t ReleaseMouseDevice(
        IInputRuntime& runtime, std::int32_t incoming_result = 0);
    [[nodiscard]] std::int32_t AcquireKeyboardDevice(IInputRuntime& runtime);
    [[nodiscard]] std::int32_t ReleaseKeyboardDevice(IInputRuntime& runtime);
    [[nodiscard]] std::int32_t UpdateInputDeviceState(IInputRuntime& runtime);
    [[nodiscard]] std::uint32_t UpdateKeyOrButtonState(
        std::uint32_t code, std::uint32_t pressed, IInputRuntime& runtime);
    [[nodiscard]] std::uint32_t InitializeInputMappings();
    [[nodiscard]] std::uint32_t InitializeKeyboardMappings();
    [[nodiscard]] std::uint32_t UpdateKeyTimingInfo(
        std::uint32_t time, std::uint32_t index, std::uint32_t pressed);
    void CleanupDevices(IInputRuntime& runtime);

private:
    InputStateRecord record_;
    std::array<std::uint8_t, kBytes - InputStateRecord::kBytes> tail_{};
};

}  // namespace pal4
