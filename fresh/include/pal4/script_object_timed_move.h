#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

using TimedMovePosition = std::array<float, 3>;

// The 68-byte movement wait object at IDA 0x5e7cf0.
struct TimedObjectMoveWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 8> base{};
    std::uint32_t phase{};
    std::array<std::byte, 16> name_buffer{};
    std::uint32_t object{};
    TimedMovePosition target{};
    TimedMovePosition start{};
    float duration{};
    float elapsed{};
};
static_assert(sizeof(TimedObjectMoveWaitState) == 0x44u);
static_assert(offsetof(TimedObjectMoveWaitState, object) == 0x20u);
static_assert(offsetof(TimedObjectMoveWaitState, target) == 0x24u);
static_assert(offsetof(TimedObjectMoveWaitState, start) == 0x30u);
static_assert(offsetof(TimedObjectMoveWaitState, duration) == 0x3cu);
static_assert(offsetof(TimedObjectMoveWaitState, elapsed) == 0x40u);

// Native game-object, interpolation, string-buffer and script operations.
class IScriptObjectTimedMoveRuntime {
public:
    virtual ~IScriptObjectTimedMoveRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindByName(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual bool HasAnimatedObject(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual TimedMovePosition ReadPosition(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual TimedObjectMoveWaitState* AllocateWait(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(TimedObjectMoveWaitState& state,
                               std::uint8_t mode,
                               std::uint32_t queue) = 0;
    virtual void AssignName(TimedObjectMoveWaitState& state,
                            std::string_view name) = 0;
    virtual void FlushWait(TimedObjectMoveWaitState& state) = 0;
    virtual void AddHandler(std::uint32_t queue,
                            TimedObjectMoveWaitState* state) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupName() = 0;
    [[nodiscard]] virtual float Interpolate(float start, float end,
                                             float ratio) = 0;
    virtual void SetPosition(std::uint32_t object,
                             const TimedMovePosition& position,
                             std::uint32_t mode) = 0;
    virtual void CloseWait(TimedObjectMoveWaitState& state) = 0;
};

// Creates and advances named game-object timed movement handlers.
class ScriptObjectTimedMove final {
public:
    static constexpr std::uint32_t kWaitVtable = 0x849660u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;

    explicit ScriptObjectTimedMove(IScriptObjectTimedMoveRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e7cf0: capture the starting position and queue a timed move.
    [[nodiscard]] std::uint32_t Create(std::string_view object_name,
                                       float duration,
                                       const TimedMovePosition& target,
                                       std::uint8_t wait_mode);
    // 0x5e7ec0: interpolate and write the new position on each update.
    void Update(TimedObjectMoveWaitState& state, double delta);

private:
    IScriptObjectTimedMoveRuntime& runtime_;
};

}  // namespace pal4
