#pragma once

#include <cstdint>
#include "pal4/script_camera_query.h"

namespace pal4 {

class IScriptImmediateModeRuntime {
public:
    virtual ~IScriptImmediateModeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ScriptInstance() = 0;
    [[nodiscard]] virtual std::uint32_t SetDebuggerMode(
        std::uint32_t debugger, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class ScriptImmediateMode final {
public:
    // IDA 0x5d2fb0 / 0x5d2fc0.
    [[nodiscard]] static std::uint32_t Begin(
        IScriptImmediateModeRuntime& runtime);
    [[nodiscard]] static std::uint32_t End(
        IScriptImmediateModeRuntime& runtime);
};

class IScriptNewGameRuntime {
public:
    virtual ~IScriptNewGameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializePlayerManager() = 0;
    virtual void ResetAllPlayerStates(std::uint32_t manager) = 0;
    virtual void LoadAndProcessPlayerData(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControlInstance() = 0;
    virtual void ShutdownActorControl(std::uint32_t control) = 0;
    [[nodiscard]] virtual int InitializeActorControl(
        std::uint32_t control) = 0;
};

class ScriptNewGameHandler final {
public:
    // IDA 0x5d2fd0.
    [[nodiscard]] static int Run(IScriptNewGameRuntime& runtime);
};

class IScriptCameraRuntime : public IScriptCameraQueryRuntime {
public:
    virtual ~IScriptCameraRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ActiveCameraObject(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual int AdjustAngles(
        std::uint32_t camera, float yaw, float pitch, float roll,
        std::uint8_t option) = 0;
    [[nodiscard]] virtual int AdjustDistance(
        std::uint32_t camera, float distance, std::uint8_t option) = 0;
    [[nodiscard]] virtual int AdjustAnglesAndDistance(
        std::uint32_t camera, float yaw, float pitch, float roll,
        float distance, std::uint8_t option) = 0;
};

class ScriptCameraHandler final {
public:
    // IDA 0x5d3000 / 0x5d3030 / 0x5d3060.
    [[nodiscard]] static int AdjustYpr(
        float yaw, float pitch, float roll, int option,
        IScriptCameraRuntime& runtime);
    [[nodiscard]] static int AdjustScalar(
        float distance, int option, IScriptCameraRuntime& runtime);
    [[nodiscard]] static int AdjustYprd(
        float yaw, float pitch, float roll, float distance, int option,
        IScriptCameraRuntime& runtime);
    // IDA 0x5d30a0 / 0x5d30b0 / 0x5d30c0 / 0x5d30d0.
    [[nodiscard]] static double Distance(IScriptCameraQueryRuntime& runtime);
    [[nodiscard]] static double Yaw(IScriptCameraQueryRuntime& runtime);
    [[nodiscard]] static double Pitch(IScriptCameraQueryRuntime& runtime);
    [[nodiscard]] static double Roll(IScriptCameraQueryRuntime& runtime);
};

}  // namespace pal4
