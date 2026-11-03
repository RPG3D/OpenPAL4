#include "pal4/game_object_animation_render.h"

namespace pal4 {

std::uint32_t GameObjectAnimationRender::Process(
    GameObjectEmbeddedAnimationState& state, const std::uint32_t source,
    const std::int32_t frame_index,
    IGameObjectAnimationRenderRuntime& runtime) {
    auto& words = state.words;
    if (words[15] != 0) return words[15];
    const auto count = static_cast<std::int32_t>(words[25]);
    if (source == 0 || words[29] == 0 || words[30] == 0 ||
        frame_index < 0 || frame_index >= count || count < 2)
        return 0;

    const auto vertices = words[29];
    const auto initial_position = runtime.FramePosition(source, frame_index);
    runtime.WriteVertexPosition(vertices, 0, initial_position);
    words[27] = 1;
    if ((runtime.FrameFlags(source, frame_index) & 1u) != 0) {
        runtime.WriteVertexPosition(vertices, words[27], initial_position);
        ++words[27];
    } else {
        std::int32_t depth = 0;
        for (auto next = frame_index + 1; next < count; ++next) {
            const auto flags = runtime.FrameFlags(source, next);
            if (depth == 0) {
                runtime.WriteVertexPosition(
                    vertices, words[27], runtime.FramePosition(source, next));
                ++words[27];
                if ((flags & 2u) == 0) break;
            }
            if ((flags & 2u) != 0) ++depth;
            if ((flags & 1u) != 0) --depth;
            if (depth < 0) break;
        }
    }

    (void)runtime.SetRenderState(6, 0);
    (void)runtime.SetRenderState(1, 0);
    if (runtime.Im3DTransform(vertices, words[27], 0, 26)) {
        runtime.RenderIndexedPrimitive(1, words[30], 2u * words[27] - 2u);
        runtime.Im3DEnd();
    }
    return runtime.SetRenderState(6, 1);
}

}  // namespace pal4
