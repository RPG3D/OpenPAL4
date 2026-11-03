#pragma once

#include "pal4/pal_actor_control_renderer.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ActorControlCircleVertex {
    float x{};
    float y{};
    float z{};
    std::array<std::uint8_t, 12> unobserved{};
    std::uint32_t color{};
    std::array<std::uint8_t, 8> tail{};
};
static_assert(sizeof(ActorControlCircleVertex) == 36u);
static_assert(offsetof(ActorControlCircleVertex, color) == 0x18u);

class IActorControlCircleRuntime {
public:
    virtual ~IActorControlCircleRuntime() = default;
    virtual void SinCos(float angle, float& sine, float& cosine);
    virtual void SetRenderState(std::uint32_t state, std::uint32_t value);
    [[nodiscard]] virtual std::uint32_t Transform(
        const ActorControlCircleVertex* vertices, std::uint32_t count,
        std::uint32_t flags);
    virtual void RenderPrimitive(std::uint32_t kind);
    [[nodiscard]] virtual std::uint32_t End3D();
};

class PalActorControlCircleRenderer final {
public:
    static constexpr std::size_t kVertexCount = 21u;
    static constexpr std::uint32_t kColor = 0xc8ff0000u;

    // IDA 0x4263d0.
    [[nodiscard]] static std::uint32_t Draw(
        const ActorControlCircularEffect& effect,
        IActorControlCircleRuntime& runtime);
};

}  // namespace pal4
