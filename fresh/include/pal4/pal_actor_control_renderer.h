#pragma once

#include "pal4/pal_actor_control_transform.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct ActorControlCircularEffect {
    ActorControlVector3 position{};
    float radius = 0.0f;
};
static_assert(sizeof(ActorControlCircularEffect) == 16u);

struct ActorControlRenderGlobals {
    ActorControlCircularEffect upper{};
    ActorControlCircularEffect lower{};
};

class IActorControlRenderRuntime {
public:
    virtual ~IActorControlRenderRuntime() = default;
    virtual void SetRenderState(std::uint32_t state,
                                std::uint32_t value) = 0;
    [[nodiscard]] virtual bool TransformLine(
        const std::uint32_t* vertices, std::uint32_t count,
        std::uint32_t flags) = 0;
    virtual void RenderIndexedPrimitive(
        std::uint32_t kind,
        const std::array<std::uint16_t, 2>& indices) = 0;
    virtual void End3D() = 0;
    [[nodiscard]] virtual double ActorDoubleProperty(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual ActorControlVector3 ActorPosition(
        std::uint32_t actor) = 0;
    virtual void RenderCircularEffect(
        const ActorControlCircularEffect& effect) = 0;
};

class ActorControlRenderRuntimeStub final
    : public IActorControlRenderRuntime {
public:
    void SetRenderState(std::uint32_t, std::uint32_t) override;
    bool TransformLine(const std::uint32_t*, std::uint32_t,
                       std::uint32_t) override;
    void RenderIndexedPrimitive(
        std::uint32_t,
        const std::array<std::uint16_t, 2>&) override;
    void End3D() override;
    double ActorDoubleProperty(std::uint32_t) override;
    ActorControlVector3 ActorPosition(std::uint32_t) override;
    void RenderCircularEffect(const ActorControlCircularEffect&) override;
};

class PalActorControlRenderer final {
public:
    // IDA 0x4245e0. Globals model original dword_8E4154 and dword_8E4164.
    static void Draw(PalActorControlState& state,
                     ActorControlRenderGlobals& globals,
                     IActorControlRenderRuntime& runtime);
};

}  // namespace pal4
