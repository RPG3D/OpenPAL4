#pragma once

#include "pal4/script_object_timed_move.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

// Scale uses the same 68-byte object/target/start/duration offsets as move.
using TimedObjectScaleWaitState = TimedObjectMoveWaitState;

// Native game-object scale, interpolation and script-queue operations.
class IScriptObjectTimedScaleRuntime {
public:
    virtual ~IScriptObjectTimedScaleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindByName(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual bool HasAnimatedObject(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual TimedMovePosition ReadScale(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual TimedObjectScaleWaitState* AllocateWait(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(TimedObjectScaleWaitState& state,
                               std::uint8_t mode,
                               std::uint32_t queue) = 0;
    virtual void FlushWait(TimedObjectScaleWaitState& state) = 0;
    [[nodiscard]] virtual std::uint32_t AddHandler(
        std::uint32_t queue, TimedObjectScaleWaitState* state) = 0;
    [[nodiscard]] virtual float Interpolate(float start, float end,
                                             float ratio) = 0;
    virtual void SetScale(std::uint32_t object,
                          const TimedMovePosition& scale,
                          std::uint32_t mode) = 0;
    virtual void CloseWait(TimedObjectScaleWaitState& state) = 0;
};

// Creates and advances a uniform game-object scale animation.
class ScriptObjectTimedScale final {
public:
    static constexpr std::uint32_t kWaitVtable = 0x849678u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;

    explicit ScriptObjectTimedScale(IScriptObjectTimedScaleRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e8160: capture XYZ scale and queue a uniform target scale.
    [[nodiscard]] std::uint32_t Create(std::string_view object_name,
                                       float target_scale,
                                       float duration,
                                       std::uint8_t wait_mode);
    // 0x5e8270: interpolate XYZ scale while duration is positive.
    void Update(TimedObjectScaleWaitState& state, double delta);

private:
    IScriptObjectTimedScaleRuntime& runtime_;
};

}  // namespace pal4
