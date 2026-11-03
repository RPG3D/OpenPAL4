#pragma once

#include <cstdint>

namespace pal4 {

class IScriptWorldMapStateRuntime {
public:
    virtual ~IScriptWorldMapStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SetExternalButtonVisibility(
        std::uint32_t name, std::int32_t state) = 0;
};

class ScriptWorldMapState final {
public:
    // IDA 0x5d5880, giWorldMapSetState thunk.
    [[nodiscard]] static std::uint32_t Set(
        std::uint32_t name, std::int32_t state,
        IScriptWorldMapStateRuntime& runtime);
};

class IScriptPuzzleResultRuntime {
public:
    virtual ~IScriptPuzzleResultRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
};

class ScriptPuzzleResult final {
public:
    // IDA 0x5d5890, giGetPuzzleGameResult.
    [[nodiscard]] static std::uint32_t Get(
        IScriptPuzzleResultRuntime& runtime);
};

class IScriptAlwaysJumpRuntime {
public:
    virtual ~IScriptAlwaysJumpRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class ScriptAlwaysJump final {
public:
    // IDA 0x5d58a0, giAlwaysJump.
    [[nodiscard]] static std::uint32_t Set(
        std::uint32_t enabled, IScriptAlwaysJumpRuntime& runtime);
};

}  // namespace pal4
