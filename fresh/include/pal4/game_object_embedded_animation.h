#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct GameObjectEmbeddedAnimationState final {
    std::array<std::uint32_t, 31> words{};
};
static_assert(sizeof(GameObjectEmbeddedAnimationState) == 0x7cu);

class IGameObjectEmbeddedAnimationRuntime {
public:
    virtual ~IGameObjectEmbeddedAnimationRuntime() = default;
    virtual void InitializeAnimationData(
        GameObjectEmbeddedAnimationState& state) = 0;
    virtual void CleanupAnimationData(
        GameObjectEmbeddedAnimationState& state) = 0;
    virtual void CleanupAnimationDataRange(
        GameObjectEmbeddedAnimationState& state) = 0;
    [[nodiscard]] virtual std::uint32_t AnimationArrayCount(
        std::uint32_t address) = 0;
    virtual void DestructAnimationArray(
        std::uint32_t address, std::uint32_t element_size,
        std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint32_t FreeMemory(
        std::uint32_t address) = 0;
    virtual void FreeWrappedMemory(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindHierarchy(
        std::uint32_t clump) = 0;
    virtual void ProcessAnimationData(
        GameObjectEmbeddedAnimationState& state,
        std::uint32_t hierarchy, std::int32_t index) = 0;
};

class IGameObjectInterpolatorRuntime
    : public IGameObjectEmbeddedAnimationRuntime {
public:
    virtual ~IGameObjectInterpolatorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t HierarchyInterpolator(
        std::uint32_t hierarchy) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentAnimation(
        std::uint32_t interpolator) = 0;
    [[nodiscard]] virtual float AnimationDuration(
        std::uint32_t animation) = 0;
    [[nodiscard]] virtual std::uint32_t RecordAnimationPointer(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual float InterpolatorTime(
        std::uint32_t interpolator) = 0;
    virtual void SetInterpolatorTime(std::uint32_t interpolator,
                                     float time) = 0;
    virtual void AddInterpolatorTime(std::uint32_t interpolator,
                                     float delta_time) = 0;
    virtual void UpdateAnimationTime(
        GameObjectEmbeddedAnimationState& state, float time) = 0;
    virtual void CleanupCurrentRecord(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t NextActionName(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyActionName() = 0;
    virtual void DispatchNextAction(
        GameObjectEmbeddedAnimationState& state,
        std::uint32_t name, float blend_time) = 0;
    virtual void BlendAnimations(
        std::uint32_t interpolator, std::uint32_t from,
        std::uint32_t to, float weight) = 0;
    [[nodiscard]] virtual std::uint32_t AnimationFromInterpolator(
        std::uint32_t interpolator) = 0;
    virtual void SetInterpolatorAnimation(
        std::uint32_t interpolator, std::uint32_t animation) = 0;
    virtual void UpdateHierarchyMatrices(std::uint32_t hierarchy) = 0;
};

class IGameObjectAnimationRenderBufferRuntime {
public:
    virtual ~IGameObjectAnimationRenderBufferRuntime() = default;
    [[nodiscard]] virtual std::int32_t FrameCount(std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void WriteHalf(std::uint32_t address, std::uint16_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class GameObjectEmbeddedAnimation final {
public:
    static constexpr std::uint32_t kVtableAddress = 0x84b948u;

    [[nodiscard]] static GameObjectEmbeddedAnimationState& InitializeState(
        GameObjectEmbeddedAnimationState& state, std::uint8_t stack_byte,
        IGameObjectEmbeddedAnimationRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupState(
        GameObjectEmbeddedAnimationState& state,
        IGameObjectEmbeddedAnimationRuntime& runtime);
    [[nodiscard]] static bool CleanupData(
        GameObjectEmbeddedAnimationState& state,
        IGameObjectEmbeddedAnimationRuntime& runtime);
    [[nodiscard]] static bool ProcessAnimation(
        GameObjectEmbeddedAnimationState& state,
        std::uint32_t argument,
        IGameObjectEmbeddedAnimationRuntime& runtime);
    [[nodiscard]] static bool UpdateInterpolator(
        GameObjectEmbeddedAnimationState& state, float delta_time,
        IGameObjectInterpolatorRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetRestartFlag(
        GameObjectEmbeddedAnimationState& state,
        std::uint32_t value) noexcept;
    [[nodiscard]] static std::uint32_t SetTimeScaleBits(
        GameObjectEmbeddedAnimationState& state,
        std::uint32_t value) noexcept;
    [[nodiscard]] static std::uint32_t SetPauseFlag(
        GameObjectEmbeddedAnimationState& state,
        std::uint32_t value) noexcept;
    [[nodiscard]] static bool SetTime(
        GameObjectEmbeddedAnimationState& state, float time,
        IGameObjectInterpolatorRuntime& runtime);
    [[nodiscard]] static std::uint32_t CompletedFlag(
        const GameObjectEmbeddedAnimationState& state) noexcept;
    [[nodiscard]] static double AnimationLength(
        const GameObjectEmbeddedAnimationState& state,
        IGameObjectInterpolatorRuntime& runtime);
    [[nodiscard]] static double AnimationTime(
        const GameObjectEmbeddedAnimationState& state,
        IGameObjectInterpolatorRuntime& runtime);
    [[nodiscard]] static bool InitializeRenderBuffers(
        GameObjectEmbeddedAnimationState& state, std::uint32_t source,
        IGameObjectAnimationRenderBufferRuntime& runtime);
};

}  // namespace pal4
