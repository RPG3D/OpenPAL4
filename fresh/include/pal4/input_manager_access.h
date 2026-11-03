#pragma once

#include <cstdint>

#include "pal4/input_manager.h"

namespace pal4 {

class IInputManagerAccessRuntime {
public:
    virtual ~IInputManagerAccessRuntime() = default;
    virtual void RegisterExit(std::uint32_t callback_address) = 0;
};

class InputManagerAccess final {
public:
    static constexpr std::uint32_t kManagerAddress = 0x8e1420u;
    static constexpr std::uint32_t kGuardAddress = 0x8e3d78u;
    static constexpr std::uint32_t kCleanupAddress = 0x4080a0u;
    static constexpr std::uint32_t kKeyCodeTableAddress = 0x8a1790u;
    static constexpr std::uint32_t kKeyCodeCount = 163u;

    InputManagerAccess(InputManager& manager, std::uint8_t& guard)
        : manager_(manager), guard_(guard) {}

    [[nodiscard]] InputManager& Get(IInputManagerAccessRuntime& runtime);
    void Cleanup(IInputRuntime& runtime);
    [[nodiscard]] std::uint32_t ProcessInputs(
        IInputRuntime& input_runtime,
        IInputManagerAccessRuntime& access_runtime);
    [[nodiscard]] static std::uint16_t KeyCodeAt(std::uint32_t index);
    [[nodiscard]] static std::uint32_t TimeGetTime(IInputRuntime& runtime);

private:
    InputManager& manager_;
    std::uint8_t& guard_;
};

}  // namespace pal4
