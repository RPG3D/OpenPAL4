#include "pal4/camera_render_geometry_callback.h"

#include <bit>

namespace pal4 {
namespace {

void Stage(ICameraRenderGeometryCallbackRuntime& runtime,
           const std::uint32_t stage, const std::uint32_t state,
           const std::uint32_t value) {
    runtime.SetTextureStageState(stage, state, value);
}

void ConfigureStageZero(ICameraRenderGeometryCallbackRuntime& runtime,
                        const bool textured) {
    if (textured) {
        Stage(runtime, 0u, 1u, 4u);
        Stage(runtime, 0u, 2u, 2u);
        Stage(runtime, 0u, 3u, 0u);
        Stage(runtime, 0u, 4u, 4u);
        Stage(runtime, 0u, 5u, 2u);
        Stage(runtime, 0u, 6u, 0u);
    } else {
        runtime.SetTexture(0u, 0u);
        Stage(runtime, 0u, 1u, 3u);
        Stage(runtime, 0u, 3u, 0u);
        Stage(runtime, 0u, 4u, 3u);
        Stage(runtime, 0u, 6u, 0u);
    }
}

std::uint32_t PackColor(ICameraRenderGeometryCallbackRuntime& runtime,
                        const std::uint32_t color) {
    const auto red = runtime.ReadByte(color);
    const auto green = runtime.ReadByte(color + 1u);
    const auto blue = runtime.ReadByte(color + 2u);
    const auto alpha = runtime.ReadByte(color + 3u);
    return (static_cast<std::uint32_t>(alpha) << 24u) |
           (static_cast<std::uint32_t>(red) << 16u) |
           (static_cast<std::uint32_t>(green) << 8u) |
           static_cast<std::uint32_t>(blue);
}

}  // namespace

void CameraRenderGeometryCallback::Render(
    const std::uint32_t render_data,
    const std::uint32_t clipping,
    const std::uint8_t clipping_flags,
    const std::uint32_t render_flags,
    const CameraProjectionGlobals& projection,
    ICameraRenderGeometryCallbackRuntime& runtime) {
    runtime.SetPixelShader(0u);
    runtime.EnableClipping(clipping, clipping_flags);
    const auto indices = runtime.ReadWord(render_data + 32u);
    if (indices != 0u) runtime.SetIndices(indices);
    runtime.SetStreams(render_data + 40u,
                       runtime.ReadWord(render_data + 72u));
    runtime.SetVertexDeclaration(runtime.ReadWord(render_data + 76u));
    const auto render_state = runtime.GetRenderState(137u);
    auto alpha_state = runtime.GetDeviceState(12u);
    const auto textured = (render_flags & 0x84u) != 0u;

    ConfigureStageZero(runtime, textured);
    runtime.SetTexture(projection.texture, 1u);
    Stage(runtime, 1u, 1u, 4u);
    const auto strength = std::bit_cast<float>(projection.strength_bits);
    Stage(runtime, 1u, 2u, strength > 0.0f ? 2u : 18u);
    Stage(runtime, 1u, 3u, 1u);
    Stage(runtime, 1u, 4u, 3u);
    Stage(runtime, 1u, 6u, 1u);
    Stage(runtime, 1u, 24u, 2u);
    Stage(runtime, 1u, 11u, 0x20000u);
    const auto use_stage_two = render_state == 0u &&
                               (render_flags & 0x40u) != 0u;
    if (use_stage_two) {
        Stage(runtime, 2u, 1u, 4u);
        Stage(runtime, 2u, 2u, 3u);
        Stage(runtime, 2u, 3u, 1u);
        Stage(runtime, 2u, 4u, 4u);
        Stage(runtime, 2u, 5u, 3u);
        Stage(runtime, 2u, 6u, 1u);
    }

    const auto count = runtime.ReadWord(render_data + 28u);
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto mesh = render_data + 100u + index * 36u;
        const auto material = runtime.ReadWord(mesh - 4u);
        const auto needs_alpha = runtime.ReadByte(material + 7u) != 0xffu ||
                                 runtime.ReadWord(mesh) != 0u;
        if (needs_alpha && alpha_state == 0u) {
            alpha_state = 1u;
            runtime.SetDeviceState(12u, 1u);
        } else if (!needs_alpha && alpha_state != 0u) {
            alpha_state = 0u;
            runtime.SetDeviceState(12u, 0u);
        }
        if (render_state != 0u) {
            runtime.SetSurfaceProperties(material + 12u,
                                         material + 4u, render_flags);
        } else if ((render_flags & 0x40u) != 0u) {
            runtime.SetRenderState(60u, PackColor(runtime, material + 4u));
        }
        if (textured)
            runtime.SetTexture(runtime.ReadWord(material), 0u);
        runtime.SetVertexShader(runtime.ReadWord(mesh + 4u));
        const auto primitive = runtime.ReadWord(render_data + 36u);
        if (indices != 0u) {
            runtime.DrawIndexed(primitive, runtime.ReadWord(mesh + 8u),
                                0u, runtime.ReadWord(mesh + 12u),
                                runtime.ReadWord(mesh + 16u),
                                runtime.ReadWord(mesh + 20u));
        } else {
            runtime.DrawPrimitive(primitive, runtime.ReadWord(mesh + 8u),
                                  runtime.ReadWord(mesh + 12u));
        }
    }

    runtime.SetTexture(0u, 1u);
    Stage(runtime, 1u, 1u, 1u);
    Stage(runtime, 1u, 4u, 1u);
    Stage(runtime, 1u, 24u, 0u);
    Stage(runtime, 1u, 11u, 1u);
    if (use_stage_two) {
        Stage(runtime, 2u, 1u, 1u);
        runtime.SetTextureStageState(2u, 4u, 1u);
    }
}

}  // namespace pal4
