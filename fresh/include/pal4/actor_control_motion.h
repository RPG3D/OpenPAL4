#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

using MotionVector = std::array<float, 3>;

struct ActorControlMotionState {
    std::uint32_t vtable{};
    std::uint32_t active{};
    std::uint32_t reserved08{};
    std::array<std::uint32_t, 3> target_position{};
    std::array<std::uint32_t, 3> final_position{};
    std::array<std::uint32_t, 3> direction{};
    std::uint32_t mode{};
    std::uint32_t pending{};
    std::uint32_t reached{};
    std::uint32_t speed_bits{};
    std::uint32_t threshold_bits{};
    std::uint32_t alternate_height_bits{};
};
static_assert(sizeof(ActorControlMotionState) == 72u);
static_assert(offsetof(ActorControlMotionState, mode) == 48u);
static_assert(offsetof(ActorControlMotionState, speed_bits) == 60u);

class IActorControlMotionRuntime {
public:
    virtual ~IActorControlMotionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FreeAddress(
        ActorControlMotionState* state) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual MotionVector ConvertPosition(
        const std::array<std::uint32_t, 3>& raw) = 0;
    [[nodiscard]] virtual bool PositionsDiffer(
        const MotionVector& left, const MotionVector& right) = 0;
    [[nodiscard]] virtual std::uint32_t GameArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t CollisionProbe(
        std::uint32_t arena, const MotionVector& target) = 0;
    [[nodiscard]] virtual bool TryMove(
        std::uint32_t collision, const MotionVector& current,
        const MotionVector& target, std::uint32_t probe,
        std::uint32_t mode) = 0;
    virtual void SetMovementSpeed(std::uint32_t collision,
                                  std::uint32_t bits) = 0;
    virtual void SetMovementTarget(std::uint32_t collision,
                                   const MotionVector& target) = 0;
    virtual void ActorCommand(std::uint32_t controller,
                              const char* command) = 0;
    [[nodiscard]] virtual bool CollisionBlocked(
        std::uint32_t collision) = 0;
    virtual void SnapActorPosition(std::uint32_t actor,
                                   const MotionVector& position,
                                   std::uint32_t mode) = 0;
    [[nodiscard]] virtual float VectorMagnitude(
        const MotionVector& delta) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeActorControl() = 0;
    virtual void ResetActorControl(std::uint32_t control) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    virtual void ProcessUiEvents(std::uint32_t manager,
                                 std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t MonsterManager() = 0;
    virtual void ClearMonsterFlags(std::uint32_t manager) = 0;
    virtual void UpdateActorPosition(std::uint32_t actor,
                                     const MotionVector& position) = 0;
    virtual void NormalizeDirection(MotionVector& direction) = 0;
    [[nodiscard]] virtual std::uint32_t SetMonsterFlags(
        std::uint32_t manager) = 0;
};

class ActorControlMotionRuntimeStub : public IActorControlMotionRuntime {
public:
    [[nodiscard]] std::uint32_t FreeAddress(
        ActorControlMotionState* state) override;
    [[nodiscard]] std::uint32_t ReadDword(
        std::uint32_t address) override;
    [[nodiscard]] MotionVector ConvertPosition(
        const std::array<std::uint32_t, 3>& raw) override;
    [[nodiscard]] bool PositionsDiffer(
        const MotionVector& left, const MotionVector& right) override;
    [[nodiscard]] std::uint32_t GameArenaManager() override;
    [[nodiscard]] std::uint32_t CollisionProbe(
        std::uint32_t arena, const MotionVector& target) override;
    [[nodiscard]] bool TryMove(
        std::uint32_t collision, const MotionVector& current,
        const MotionVector& target, std::uint32_t probe,
        std::uint32_t mode) override;
    void SetMovementSpeed(std::uint32_t collision,
                          std::uint32_t bits) override;
    void SetMovementTarget(std::uint32_t collision,
                           const MotionVector& target) override;
    void ActorCommand(std::uint32_t controller,
                      const char* command) override;
    [[nodiscard]] bool CollisionBlocked(
        std::uint32_t collision) override;
    void SnapActorPosition(std::uint32_t actor,
                           const MotionVector& position,
                           std::uint32_t mode) override;
    [[nodiscard]] float VectorMagnitude(
        const MotionVector& delta) override;
    [[nodiscard]] std::uint32_t InitializeActorControl() override;
    void ResetActorControl(std::uint32_t control) override;
    [[nodiscard]] std::uint32_t UiFrameManager() override;
    void ProcessUiEvents(std::uint32_t manager,
                         std::uint32_t mode) override;
    [[nodiscard]] std::uint32_t MonsterManager() override;
    void ClearMonsterFlags(std::uint32_t manager) override;
    void UpdateActorPosition(std::uint32_t actor,
                             const MotionVector& position) override;
    void NormalizeDirection(MotionVector& direction) override;
    [[nodiscard]] std::uint32_t SetMonsterFlags(
        std::uint32_t manager) override;
};

class ActorControlMotion final {
public:
    static constexpr std::uint32_t kVtable = 0x8425acu;
    static constexpr std::uint32_t kBaseVtable = 0x8425d4u;
    static constexpr std::uint32_t kInitialSpeedBits = 0x42200000u;

    [[nodiscard]] static ActorControlMotionState& Construct(
        ActorControlMotionState& state);
    [[nodiscard]] static ActorControlMotionState* DeleteBase(
        ActorControlMotionState* state, std::uint32_t flags,
        IActorControlMotionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Reset(
        ActorControlMotionState& state);
    [[nodiscard]] static std::uint32_t ResetWithArgument(
        ActorControlMotionState& state, std::uint32_t ignored);
    [[nodiscard]] static std::uint32_t Update(
        ActorControlMotionState& state, std::uint32_t actor,
        float delta_seconds, IActorControlMotionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Begin(
        ActorControlMotionState& state,
        const std::array<std::uint32_t, 3>& start,
        const std::array<std::uint32_t, 3>& finish,
        std::uint32_t alternate_height_bits, std::uint32_t reached,
        IActorControlMotionRuntime& runtime);
};

}  // namespace pal4
