#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

using NpcFlyVector = std::array<float, 3>;

// The 84-byte NPC fly wait handler installed by 0x5e88b0.
struct NpcFlyWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 8> base{};
    std::uint32_t phase{};
    std::array<std::byte, 16> name_buffer{};
    std::uint32_t npc{};
    NpcFlyVector target{};
    NpcFlyVector start{};
    float speed{};
    std::array<std::byte, 12> reserved{};
    float duration{};
    float elapsed{};
};
static_assert(sizeof(NpcFlyWaitState) == 0x54u);
static_assert(offsetof(NpcFlyWaitState, npc) == 0x20u);
static_assert(offsetof(NpcFlyWaitState, target) == 0x24u);
static_assert(offsetof(NpcFlyWaitState, start) == 0x30u);
static_assert(offsetof(NpcFlyWaitState, speed) == 0x3cu);
static_assert(offsetof(NpcFlyWaitState, duration) == 0x4cu);
static_assert(offsetof(NpcFlyWaitState, elapsed) == 0x50u);

// Native NPC, PAL4 vector math, animation and script-queue operations.
class IScriptNpcFlyRuntime {
public:
    virtual ~IScriptNpcFlyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindNpc(
        std::uint32_t manager, std::string_view name) = 0;
    virtual void SetAnimation(std::uint32_t npc,
                              std::string_view animation) = 0;
    virtual void SetAnimationFlag(std::uint32_t npc,
                                  std::uint32_t flag) = 0;
    [[nodiscard]] virtual NpcFlyVector ReadPosition(
        std::uint32_t npc) = 0;
    [[nodiscard]] virtual NpcFlyWaitState* AllocateWait(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(NpcFlyWaitState& state,
                               std::uint8_t mode,
                               std::uint32_t queue) = 0;
    virtual void AssignName(NpcFlyWaitState& state,
                            std::string_view name) = 0;
    [[nodiscard]] virtual float DistanceFromVector(
        const NpcFlyVector& vector) = 0;
    virtual void ClampFacingVector(NpcFlyVector& vector) = 0;
    [[nodiscard]] virtual float AngleFromZ(float normalized_z) = 0;
    virtual void SetRotation(std::uint32_t npc,
                             const NpcFlyVector& rotation,
                             std::uint32_t mode) = 0;
    virtual void FlushWait(NpcFlyWaitState& state) = 0;
    virtual void AddHandler(std::uint32_t queue,
                            NpcFlyWaitState* state) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupName() = 0;
    [[nodiscard]] virtual float Interpolate(float start, float end,
                                             float ratio) = 0;
    virtual void SetPosition(std::uint32_t npc,
                             const NpcFlyVector& position) = 0;
    virtual void CloseWait(NpcFlyWaitState& state) = 0;
    [[nodiscard]] virtual std::int32_t SetIdleAction(
        std::uint32_t npc, std::string_view action) = 0;
};

// Scripted NPC fly movement and the two vtable callbacks.
class ScriptNpcFlyMove final {
public:
    static constexpr std::uint32_t kWaitVtable = 0x849690u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;
    static constexpr float kSpeed = 69.0f;

    explicit ScriptNpcFlyMove(IScriptNpcFlyRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e88b0: set flying animation, target and initial yaw.
    [[nodiscard]] std::uint32_t Create(std::string_view npc_name,
                                       const NpcFlyVector& target,
                                       std::uint8_t wait_mode);
    // 0x5e8b30: interpolate NPC XYZ until duration is reached.
    void Update(NpcFlyWaitState& state, double delta);
    // 0x5e8be0: close wait then select C01 idle animation.
    [[nodiscard]] std::int32_t Finish(NpcFlyWaitState& state);

private:
    IScriptNpcFlyRuntime& runtime_;
};

}  // namespace pal4
