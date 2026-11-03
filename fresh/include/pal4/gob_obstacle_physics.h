#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct GobObstacleVector {
    float x;
    float y;
    float z;
};

struct GobObstacleEffectHandle {
    std::uint32_t first;
    std::uint32_t second;
};

class IGobObstaclePhysicsRuntime {
public:
    virtual ~IGobObstaclePhysicsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t address) = 0;
    virtual void UpdateMachine(std::uint32_t object, float elapsed) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    [[nodiscard]] virtual double ActorHeight(std::uint32_t actor) = 0;
    [[nodiscard]] virtual float HorizontalMagnitude(
        const GobObstacleVector& vector) = 0;
    virtual void ClampVectorToRange(float range,
                                    GobObstacleVector& vector) = 0;
    virtual void ApplyActorWorldForce(const GobObstacleVector& force) = 0;
    virtual void TickFloorSound(float elapsed) = 0;
    [[nodiscard]] virtual GobObstacleVector ObjectForward(
        std::uint32_t object) = 0;
    virtual void ApplyPlayerStatus(float duration) = 0;
    virtual void StartPlayerStatus(std::uint32_t first,
                                    std::uint32_t second,
                                    float duration) = 0;
    virtual void BeginMovementStatus(std::uint32_t value) = 0;
    [[nodiscard]] virtual bool DialogNeedsUpdate(std::uint32_t handle) = 0;
    virtual void UpdateDialogContent(std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t FindObjectByName(
        std::string_view name) = 0;
    virtual void RemoveAnimatedObject(std::uint32_t object) = 0;
    virtual void ResetAnimatedObject(std::uint32_t object) = 0;
    virtual void PositionAnimatedObject(std::uint32_t object,
                                        std::uint32_t position) = 0;
    virtual void SetAnimatedObjectVisibility(std::uint32_t object,
                                             std::uint32_t visible) = 0;
    virtual void OrientAnimatedObject(std::uint32_t object,
                                      std::uint32_t position) = 0;
    virtual void SetAllAtomicMaterialFlags(std::uint32_t object,
                                           std::uint32_t flags) = 0;
    virtual void AddAnimatedObject(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t TimeMilliseconds() = 0;
    [[nodiscard]] virtual float RandomDelay(float minimum, float maximum,
                                            float seed) = 0;
    [[nodiscard]] virtual GobObstacleEffectHandle CreateEffect(
        std::string_view name) = 0;
    virtual void ResetTemporaryEffect() = 0;
    [[nodiscard]] virtual std::uint32_t UpdateEffectProperties(
        std::uint32_t handle, std::uint32_t clump) = 0;
};

}  // namespace pal4
