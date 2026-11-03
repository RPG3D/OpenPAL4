#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptPostEventRuntime {
public:
    virtual ~IScriptPostEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t QueryCurrentState(
        std::uint32_t state) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeStateAndUi(
        std::uint32_t game, std::uint32_t state,
        std::uint32_t document) = 0;
    [[nodiscard]] virtual std::uint32_t TradeSystem() = 0;
    [[nodiscard]] virtual std::uint32_t SetTradeItemInfo(
        std::uint32_t trade, std::string_view id,
        std::uint32_t flag) = 0;
};

class ScriptPostEventHandler final {
public:
    // IDA 0x5d3d80.
    [[nodiscard]] static std::uint8_t Handle(
        std::uint32_t mode, IScriptPostEventRuntime& runtime);
};

class IMonsterPursuitCommandRuntime {
public:
    using Callback = std::uint32_t (*)(
        std::uint32_t monster, IMonsterPursuitCommandRuntime& runtime);
    virtual ~IMonsterPursuitCommandRuntime() = default;
    virtual void SetStopPursuitFlag(std::uint32_t monster) = 0;
    [[nodiscard]] virtual std::uint32_t MonsterManager() = 0;
    [[nodiscard]] virtual std::uint32_t Iterate(
        std::uint32_t manager, Callback callback,
        std::uint32_t context) = 0;
};

class MonsterPursuitController final {
public:
    // IDA 0x5d3e00 and 0x5d3e10.
    [[nodiscard]] static std::uint32_t StopOne(
        std::uint32_t monster, IMonsterPursuitCommandRuntime& runtime);
    [[nodiscard]] static std::uint32_t StopAll(
        IMonsterPursuitCommandRuntime& runtime);
};

class IScriptCameraModeRuntime {
public:
    virtual ~IScriptCameraModeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CameraSubsystem() = 0;
    virtual void SetModeByIndex(std::uint32_t manager,
                                std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCameraInternal(
        std::uint32_t manager) = 0;
    virtual void SetPosition(std::uint32_t camera,
                             std::uint32_t x, std::uint32_t y,
                             std::uint32_t z, std::uint32_t keep_look_at) = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCameraObject(
        std::uint32_t manager) = 0;
    virtual void SetLookAt(std::uint32_t camera,
                           std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    virtual void FlushMainCameraTailYaw(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::int32_t SetPlayerState(
        std::uint32_t manager, std::uint8_t sync) = 0;
};

class ScriptCameraModeHandler final {
public:
    // IDA 0x5d3e30.
    [[nodiscard]] static std::int32_t SetMode(
        std::uint32_t mode, std::uint32_t sync_player_state,
        IScriptCameraModeRuntime& runtime);
};

}  // namespace pal4
