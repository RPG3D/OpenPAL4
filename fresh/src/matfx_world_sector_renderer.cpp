#include "pal4/matfx_world_sector_renderer.h"

namespace pal4 {
namespace {

using Op = WorldSectorRenderCall;
using Runtime = IWorldSectorRenderRuntime;

std::uint32_t Call(Runtime& runtime, const Op operation,
                   const std::initializer_list<std::uint32_t> arguments = {},
                   const std::string_view text = {}) {
    return runtime.Call(operation, arguments, text);
}

void StageState(Runtime& runtime, const std::uint32_t stage,
                const std::uint32_t state, const std::uint32_t value) {
    (void)Call(runtime, Op::SetTextureStageState, {stage, state, value});
}

void DrawMesh(Runtime& runtime, const std::uint32_t header,
              const std::uint32_t mesh) {
    const auto primitive = runtime.ReadWord(header + 12u);
    if (runtime.ReadWord(header + 8u) != 0u)
        (void)Call(runtime, Op::DrawIndexed,
                   {primitive, runtime.ReadWord(mesh + 12u), 0u,
                    runtime.ReadWord(mesh + 16u),
                    runtime.ReadWord(mesh + 20u),
                    runtime.ReadWord(mesh + 24u)});
    else
        (void)Call(runtime, Op::DrawPrimitive,
                   {primitive, runtime.ReadWord(mesh + 12u),
                    runtime.ReadWord(mesh + 24u)});
}

}  // namespace

void MatFxWorldSectorRenderer::Render(
    const std::uint32_t render_data, const std::uint32_t clipping,
    const std::uint32_t clipping_flags, const std::uint32_t render_flags,
    Runtime& runtime) {
    (void)Call(runtime, Op::SetPixelShader, {0u});
    (void)Call(runtime, Op::EnableClipping,
               {clipping, clipping_flags & 0xffu});
    const auto header = render_data + 24u;
    const auto indices = runtime.ReadWord(header + 8u);
    if (indices != 0u) (void)Call(runtime, Op::SetIndices, {indices});
    (void)Call(runtime, Op::SetStreams,
               {header + 16u, runtime.ReadWord(header + 48u)});
    (void)Call(runtime, Op::SetVertexDeclaration,
               {runtime.ReadWord(header + 52u)});
    const auto render_state = Call(runtime, Op::GetRenderState, {137u});
    if (render_state == 0u && (render_flags & 8u) == 0u)
        return WorldSectorLightmapPipeline::Render(
            header, (render_flags & 0x84u) != 0u, runtime);

    (void)Call(runtime, Op::GetVertexAlphaPreference);
    const auto count = runtime.ReadWord(header + 4u);
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto mesh = header + 72u + 36u * index;
        const auto material = runtime.ReadWord(mesh);
        (void)Call(runtime, Op::SetTextureStage,
                   {runtime.ReadWord(material), 0u});
        std::uint32_t dual = 0u;
        if (Call(runtime, Op::GetMaterialEffects, {material}) == 4u) {
            dual = Call(runtime, Op::GetDualTexture, {material});
            (void)Call(runtime, Op::SetTextureStage, {dual, 1u});
        }
        StageState(runtime, 0u, 2u, 2u);
        StageState(runtime, 0u, 3u, 0u);
        StageState(runtime, 0u, 1u, 4u);
        if (dual != 0u) {
            StageState(runtime, 1u, 2u, 1u);
            StageState(runtime, 1u, 3u, 2u);
            StageState(runtime, 1u, 1u, 4u);
        }
        (void)Call(runtime, Op::SetVertexShader,
                   {runtime.ReadWord(mesh + 8u)});
        if (dual != 0u) {
            if (runtime.ReadWord(0x8f94c4u) != 0u) {
                (void)Call(runtime, Op::VmSetVector,
                           {runtime.ReadWord(0x8f94c4u),
                            runtime.ReadWord(0x8f94b8u),
                            runtime.ReadWord(0x8f94bcu),
                            runtime.ReadWord(0x8f94c0u), 3u},
                           "Saturation");
                (void)Call(runtime, Op::VmSetFloat,
                           {runtime.ReadWord(0x8f94c4u),
                            runtime.ReadWord(0x8f94b4u)},
                           "Brightness");
                (void)Call(runtime, Op::VmFindTechnique,
                           {runtime.ReadWord(0x8f94c4u)}, "RtkWorldLtMap");
                const auto passes = Call(
                    runtime, Op::VmPassCount,
                    {runtime.ReadWord(0x8f94c4u), 0u});
                for (std::uint32_t pass = 0u; pass < passes; ++pass) {
                    (void)Call(runtime, Op::VmSetPass,
                               {runtime.ReadWord(0x8f94c4u), pass});
                    DrawMesh(runtime, header, mesh);
                    (void)Call(runtime, Op::VmEndPass,
                               {runtime.ReadWord(0x8f94c4u)});
                }
                (void)Call(runtime, Op::VmEndTechnique,
                           {runtime.ReadWord(0x8f94c4u)});
            }
        } else {
            DrawMesh(runtime, header, mesh);
        }
        if (dual != 0u) {
            (void)Call(runtime, Op::SetTextureStage, {0u, 1u});
            StageState(runtime, 1u, 1u, 1u);
            StageState(runtime, 1u, 4u, 1u);
        }
    }
}

}  // namespace pal4
