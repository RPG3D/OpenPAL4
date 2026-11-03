#pragma once

#include "pal4/actor_control_discrete_movement.h"
#include "pal4/actor_control_cursor_point.h"

#include <cstdint>

namespace pal4 {

struct ActorControlRay {
    ActorControlVec3 origin{};
    ActorControlVec3 direction{};
};

class IActorControlMouseProjectionRuntime {
public:
    virtual ~IActorControlMouseProjectionRuntime() = default;
    [[nodiscard]] virtual ActorControlCursorPoint CursorPosition() = 0;
    virtual void ScreenToClient(ActorControlCursorPoint& point) = 0;
    [[nodiscard]] virtual std::uint32_t CameraSystem() = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCameraInternalId(
        std::uint32_t system) = 0;
    virtual void CalculateNormalizedPixelRay(std::uint32_t camera,
                                             float x, float y,
                                             ActorControlRay& ray) = 0;
    [[nodiscard]] virtual std::uint32_t ReadActorWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual double CalculateIntersection(
        const ActorControlRay& ray, const ActorControlVec3& plane_normal,
        const ActorControlVec3& actor_position) = 0;
};

class ActorControlMouseProjectionRuntimeStub final
    : public IActorControlMouseProjectionRuntime {
public:
    ActorControlCursorPoint CursorPosition() override;
    void ScreenToClient(ActorControlCursorPoint& point) override;
    std::uint32_t CameraSystem() override;
    std::uint32_t ActiveCameraInternalId(std::uint32_t system) override;
    void CalculateNormalizedPixelRay(std::uint32_t camera, float x, float y,
                                     ActorControlRay& ray) override;
    std::uint32_t ReadActorWord(std::uint32_t address) override;
    double CalculateIntersection(const ActorControlRay& ray,
                                 const ActorControlVec3& plane_normal,
                                 const ActorControlVec3& actor_position) override;
};

class ActorControlMouseProjection final {
public:
    [[nodiscard]] static bool Project(std::uint32_t actor,
                                      ActorControlVec3& output,
                                      IActorControlMouseProjectionRuntime& runtime);
};

}  // namespace pal4
