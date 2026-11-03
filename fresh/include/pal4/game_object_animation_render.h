#pragma once

#include "pal4/game_object_embedded_animation.h"

#include <array>
#include <cstdint>

namespace pal4 {

using AnimationFramePosition = std::array<std::uint32_t, 3>;

class IGameObjectAnimationRenderRuntime {
public:
    virtual ~IGameObjectAnimationRenderRuntime() = default;
    [[nodiscard]] virtual AnimationFramePosition FramePosition(
        std::uint32_t source, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t FrameFlags(
        std::uint32_t source, std::int32_t index) = 0;
    virtual void WriteVertexPosition(
        std::uint32_t vertices, std::uint32_t index,
        const AnimationFramePosition& position) = 0;
    [[nodiscard]] virtual std::uint32_t SetRenderState(
        std::uint32_t state, std::uint32_t value) = 0;
    [[nodiscard]] virtual bool Im3DTransform(
        std::uint32_t vertices, std::uint32_t count,
        std::uint32_t matrix, std::uint32_t flags) = 0;
    virtual void RenderIndexedPrimitive(
        std::uint32_t primitive_type, std::uint32_t indices,
        std::uint32_t index_count) = 0;
    virtual void Im3DEnd() = 0;
};

class GameObjectAnimationRenderRuntimeStub final
    : public IGameObjectAnimationRenderRuntime {
public:
    [[nodiscard]] AnimationFramePosition FramePosition(
        std::uint32_t, std::int32_t) override { return {}; }
    [[nodiscard]] std::uint32_t FrameFlags(
        std::uint32_t, std::int32_t) override { return 0; }
    void WriteVertexPosition(
        std::uint32_t, std::uint32_t,
        const AnimationFramePosition&) override {}
    [[nodiscard]] std::uint32_t SetRenderState(
        std::uint32_t, std::uint32_t) override { return 0; }
    [[nodiscard]] bool Im3DTransform(
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t) override { return false; }
    void RenderIndexedPrimitive(
        std::uint32_t, std::uint32_t, std::uint32_t) override {}
    void Im3DEnd() override {}
};

class GameObjectAnimationRender final {
public:
    [[nodiscard]] static std::uint32_t Process(
        GameObjectEmbeddedAnimationState& state, std::uint32_t source,
        std::int32_t frame_index, IGameObjectAnimationRenderRuntime& runtime);
};

}  // namespace pal4
