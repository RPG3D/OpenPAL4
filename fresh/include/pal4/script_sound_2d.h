#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptSound2dRuntime {
public:
    virtual ~IScriptSound2dRuntime() = default;
    [[nodiscard]] virtual std::string_view SoundName(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlaySound(
        std::uint32_t audio, std::uint32_t source,
        std::string_view path, std::int32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    [[nodiscard]] virtual std::int32_t InsertSoundId(
        std::uint32_t executor, std::uint32_t handle) = 0;
    virtual void SetCurrentHandle(std::uint32_t address,
                                  std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t CheckActive(
        std::uint32_t handle_address) = 0;
    [[nodiscard]] virtual std::uint32_t StopCurrent(
        std::uint32_t handle_address) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveSoundId(
        std::uint32_t executor, std::int32_t id) = 0;
};

class ScriptSound2d final {
public:
    // IDA 0x5d58c0, gi2DSoundPlay.
    [[nodiscard]] static std::int32_t Play(
        std::uint32_t source, std::int32_t mode,
        IScriptSound2dRuntime& runtime);
    // IDA 0x5d5a30, gi2DSoundStop.
    [[nodiscard]] static std::uint32_t StopCurrent(
        IScriptSound2dRuntime& runtime);
    // IDA 0x5d5a60, gi2DSoundStopID.
    [[nodiscard]] static std::uint32_t StopId(
        std::int32_t id, IScriptSound2dRuntime& runtime);
};

}  // namespace pal4
