#pragma once

#include "pal4/script_object_timed_move.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

// Current-player movement shares the 68-byte object/target/start layout.
using CurrentPlayerTimedMoveWaitState = TimedObjectMoveWaitState;

// Native player animation, position, interpolation and script calls.
class IScriptCurrentPlayerTimedMoveRuntime {
public:
    virtual ~IScriptCurrentPlayerTimedMoveRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t CurrentPlayer(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual bool HasMovementComponent(
        std::uint32_t player) = 0;
    [[nodiscard]] virtual std::string_view AnimationName(
        std::uint32_t player) = 0;
    virtual void SelectAnimation(std::uint32_t player,
                                 std::string_view animation) = 0;
    virtual void SetAnimationFlag(std::uint32_t player,
                                  std::uint32_t flag) = 0;
    [[nodiscard]] virtual TimedMovePosition ReadPosition(
        std::uint32_t player) = 0;
    [[nodiscard]] virtual CurrentPlayerTimedMoveWaitState* AllocateWait(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(CurrentPlayerTimedMoveWaitState& state,
                               std::uint8_t mode,
                               std::uint32_t queue) = 0;
    virtual void FlushWait(CurrentPlayerTimedMoveWaitState& state) = 0;
    [[nodiscard]] virtual std::uint32_t AddHandler(
        std::uint32_t queue,
        CurrentPlayerTimedMoveWaitState* state) = 0;
    [[nodiscard]] virtual float Interpolate(float start, float end,
                                             float ratio) = 0;
    virtual void SetPosition(std::uint32_t player,
                             const TimedMovePosition& position) = 0;
    virtual void CloseWait(CurrentPlayerTimedMoveWaitState& state) = 0;
};

// Script-controlled timed movement for the current player.
class ScriptCurrentPlayerTimedMove final {
public:
    static constexpr std::uint32_t kWaitVtable = 0x849684u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;

    explicit ScriptCurrentPlayerTimedMove(
        IScriptCurrentPlayerTimedMoveRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e8600: switch to C01 as needed and queue a timed move.
    [[nodiscard]] std::uint32_t Create(float duration,
                                       const TimedMovePosition& target,
                                       std::uint8_t wait_mode);
    // 0x5e8760: interpolate player XYZ until duration is reached.
    void Update(CurrentPlayerTimedMoveWaitState& state, double delta);

private:
    IScriptCurrentPlayerTimedMoveRuntime& runtime_;
};

}  // namespace pal4
