#include "pal4/world_sector_lightmap_pipeline.h"

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

std::uint32_t WorldSectorLightmapPipeline::Create(Runtime& runtime) {
    const auto pipeline = Call(runtime, Op::CreatePipeline);
    const auto definition = Call(runtime, Op::GetNodeDefinition);
    (void)Call(runtime, Op::SetMipmapping, {0u});
    if (pipeline == 0u) return 0u;
    const auto locked = Call(runtime, Op::LockPipeline, {pipeline});
    if (locked != 0u &&
        Call(runtime, Op::AddFragment, {locked, 0u, definition, 0u}) != 0u &&
        Call(runtime, Op::UnlockPipeline, {locked}) != 0u) {
        const auto node = Call(runtime, Op::FindNodeByName,
                               {pipeline, runtime.ReadWord(definition), 0u, 0u});
        (void)Call(runtime, Op::SetInstanceCallback, {node});
        (void)Call(runtime, Op::SetRenderCallback, {node});
        return pipeline;
    }
    (void)Call(runtime, Op::DestroyPipeline, {pipeline});
    return 0u;
}

void WorldSectorLightmapPipeline::RenderCallback(
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
        return Render(header, (render_flags & 0x84u) != 0u, runtime);

    (void)Call(runtime, Op::GetVertexAlphaPreference);
    const auto count = runtime.ReadWord(header + 4u);
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto mesh = header + 72u + index * 36u;
        const auto material = runtime.ReadWord(mesh);
        (void)Call(runtime, Op::SetTextureStage,
                   {runtime.ReadWord(material), 0u});
        std::uint32_t dual_texture = 0u;
        if (Call(runtime, Op::GetMaterialEffects, {material}) == 4u) {
            dual_texture = Call(runtime, Op::GetDualTexture, {material});
            (void)Call(runtime, Op::SetTextureStage, {dual_texture, 1u});
        }
        StageState(runtime, 0u, 2u, 2u);
        StageState(runtime, 0u, 3u, 0u);
        StageState(runtime, 0u, 1u, 4u);
        if (dual_texture != 0u) {
            StageState(runtime, 1u, 2u, 1u);
            StageState(runtime, 1u, 3u, 2u);
            StageState(runtime, 1u, 1u, 4u);
        }
        (void)Call(runtime, Op::SetVertexShader,
                   {runtime.ReadWord(mesh + 8u)});
        if (dual_texture != 0u) {
            const auto vm = Call(runtime, Op::GetEffectVm);
            if (vm != 0u) {
                const auto red = runtime.ReadWord(
                    Call(runtime, Op::GetArenaManager) + 428u);
                const auto green = runtime.ReadWord(
                    Call(runtime, Op::GetArenaManager) + 432u);
                const auto blue = runtime.ReadWord(
                    Call(runtime, Op::GetArenaManager) + 436u);
                (void)Call(runtime, Op::VmSetVector,
                           {Call(runtime, Op::GetEffectVm), red, green, blue, 3u},
                           "Saturation");
                const auto brightness = runtime.ReadWord(
                    Call(runtime, Op::GetArenaManager) + 440u);
                (void)Call(runtime, Op::VmSetFloat,
                           {Call(runtime, Op::GetEffectVm), brightness},
                           "Brightness");
                (void)Call(runtime, Op::VmFindTechnique,
                           {Call(runtime, Op::GetEffectVm)}, "RtkWorldLtMap");
                const auto passes = Call(runtime, Op::VmPassCount,
                                         {Call(runtime, Op::GetEffectVm), 0u});
                for (std::uint32_t pass = 0u; pass < passes; ++pass) {
                    (void)Call(runtime, Op::VmSetPass,
                               {Call(runtime, Op::GetEffectVm), pass});
                    DrawMesh(runtime, header, mesh);
                    (void)Call(runtime, Op::VmEndPass,
                               {Call(runtime, Op::GetEffectVm)});
                }
                (void)Call(runtime, Op::VmEndTechnique,
                           {Call(runtime, Op::GetEffectVm)});
            }
        } else {
            DrawMesh(runtime, header, mesh);
        }
        if (dual_texture != 0u) {
            (void)Call(runtime, Op::SetTextureStage, {0u, 1u});
            StageState(runtime, 1u, 1u, 1u);
            StageState(runtime, 1u, 4u, 1u);
        }
    }
}

void WorldSectorLightmapPipeline::Render(const std::uint32_t header,
                                   const bool use_alpha_texture,
                                   Runtime& runtime) {
    const auto old_state_26 = Call(runtime, Op::GetRenderState, {26u});
    const auto old_state_9 = Call(runtime, Op::GetRenderState, {9u});
    (void)Call(runtime, Op::SetVertexAlpha, {0u});
    (void)Call(runtime, Op::SetRenderState, {60u, 0xff000000u});
    (void)Call(runtime, Op::SetRenderState, {26u, 0u});
    (void)Call(runtime, Op::SetRenderState, {9u, 1u});
    StageState(runtime, 0u, 1u, 3u);
    StageState(runtime, 0u, 2u, 2u);
    StageState(runtime, 0u, 3u, 3u);
    StageState(runtime, 0u, 4u, 3u);
    StageState(runtime, 0u, 5u, 2u);
    StageState(runtime, 0u, 6u, 3u);
    (void)Call(runtime, Op::SetTextureStage, {0u, 0u});

    bool alpha_enabled = false;
    const auto count = runtime.ReadWord(header + 4u);
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto mesh = header + 72u + index * 36u;
        const auto material = runtime.ReadWord(mesh);
        const auto texture = runtime.ReadWord(material);
        const auto has_alpha = use_alpha_texture && texture != 0u &&
                               Call(runtime, Op::TextureHasAlpha, {texture}) != 0u;
        if (has_alpha) {
            (void)Call(runtime, Op::SetTextureStage, {texture, 0u});
            if (!alpha_enabled) {
                alpha_enabled = true;
                StageState(runtime, 0u, 4u, 4u);
            }
        } else if (alpha_enabled) {
            alpha_enabled = false;
            StageState(runtime, 0u, 4u, 3u);
            (void)Call(runtime, Op::SetTextureStage, {0u, 0u});
        }
        (void)Call(runtime, Op::SetVertexShader,
                   {runtime.ReadWord(mesh + 8u)});
        DrawMesh(runtime, header, mesh);
    }
    (void)Call(runtime, Op::SetRenderState, {26u, old_state_26});
    (void)Call(runtime, Op::SetRenderState, {9u, old_state_9});
}

}  // namespace pal4
