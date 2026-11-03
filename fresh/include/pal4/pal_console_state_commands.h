#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalConsoleStateRuntime {
public:
    virtual ~IPalConsoleStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PetManager() = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    [[nodiscard]] virtual std::int32_t ActivateStatusMode(
        std::uint32_t actor_control, std::int32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::int32_t SetTimerFrameDuration(
        std::uint32_t frame_manager, std::int32_t duration) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::int32_t AddOutputLine(
        std::uint32_t console, std::string_view text) = 0;
};

class PalConsoleStateCommands final {
public:
    // IDA 0x5bec70 / 0x5bed00 / 0x5bed70.
    [[nodiscard]] static std::int32_t SetPetShow(
        const char* command, IPalConsoleStateRuntime& runtime);
    [[nodiscard]] static std::int32_t EnableSceneTransition(
        const char* command, IPalConsoleStateRuntime& runtime);
    [[nodiscard]] static std::int32_t ResetUiTimer(
        const char* command, IPalConsoleStateRuntime& runtime);
};

}  // namespace pal4
