#include "pal4/scene_sky_dual_clumps.h"

#include <initializer_list>
#include <string>

namespace pal4 {
namespace {

std::uint32_t EnsureMatrix(std::uint32_t& matrix,
                           ISceneSkyDualRuntime& runtime) {
    if (matrix == 0u) matrix = runtime.CreateMatrix();
    return matrix;
}

}  // namespace

SceneSkyDualState& SceneSkyDualClumps::Initialize(SceneSkyDualState& state) {
    state.vtable = kVtable;
    for (auto* layer : {&state.first, &state.second}) {
        layer->clump = 0u;
        layer->first_matrix = 0u;
        layer->second_matrix = 0u;
        layer->active = 0u;
    }
    return state;
}

std::uint32_t SceneSkyDualClumps::Destroy(
    SceneSkyDualState& state, ISceneSkyDualCleanupRuntime& runtime) {
    state.vtable = kVtable;
    return Cleanup(state, runtime);
}

std::uint32_t SceneSkyDualClumps::Cleanup(
    SceneSkyDualState& state, ISceneSkyDualCleanupRuntime& runtime) {
    std::uint32_t result = 0u;
    for (auto* layer : {&state.first, &state.second}) {
        if (layer->clump != 0u) {
            runtime.SetCleanupPipeline(layer->clump);
            runtime.DestroyClump(layer->clump);
            layer->clump = 0u;
        }
        if (layer->first_matrix != 0u) {
            (void)runtime.DestroyMatrix(layer->first_matrix);
            layer->first_matrix = 0u;
        }
        result = layer->second_matrix;
        if (layer->second_matrix != 0u) {
            result = runtime.DestroyMatrix(layer->second_matrix);
            layer->second_matrix = 0u;
        }
        layer->active = 0u;
    }
    return result;
}

bool SceneSkyDualClumps::Load(
    SceneSkyDualState& state, const std::string_view source,
    const std::string_view first_model,
    const std::string_view first_texture,
    const std::string_view second_model,
    const std::string_view second_texture,
    const std::uint32_t option_one,
    const std::uint32_t option_two,
    ISceneSkyDualRuntime& runtime) {
    (void)Cleanup(state, runtime);
    const std::string prefix(source);
    const auto first_path = prefix + std::string(first_model) + ".dff";
    state.first.clump = runtime.LoadClump(first_path, source);
    if (state.first.clump == 0u) return false;

    runtime.PrepareImage(source);
    const auto first_texture_object = runtime.ReadTexture(first_texture);
    if (first_texture_object != 0u)
        runtime.SetTextureFlags(first_texture_object, 2u, 17u);
    (void)EnsureMatrix(state.first.first_matrix, runtime);
    (void)EnsureMatrix(state.first.second_matrix, runtime);
    state.first.second_translation[0] = 29.8999996f;
    runtime.TranslateMatrix(state.first.second_matrix,
                            state.first.second_translation, 0u);
    const SkyClumpPipeline first_pipeline{
        first_texture_object == 0u ? 1u : 2u,
        first_texture_object, option_one, option_two,
        state.first.first_matrix, state.first.second_matrix};
    runtime.SetClumpPipeline(state.first.clump, first_pipeline);
    if (first_texture_object != 0u)
        runtime.DestroyTexture(first_texture_object);
    state.first.active = 1u;
    runtime.ClearVertexColor(state.first.clump, 32u);
    runtime.SetVertexColor(state.first.clump, 0xffffffffu);

    const auto second_path = prefix + std::string(second_model) + ".dff";
    state.second.clump = runtime.LoadClump(second_path, source);
    if (state.second.clump == 0u) return false;

    runtime.PrepareImage(source);
    const auto second_texture_object = runtime.ReadTexture(second_texture);
    if (second_texture_object != 0u)
        runtime.SetTextureFlags(second_texture_object, 2u, 17u);
    (void)EnsureMatrix(state.second.first_matrix, runtime);
    (void)EnsureMatrix(state.second.second_matrix, runtime);
    state.second.first_translation[0] = 49.9000015f;
    state.second.second_translation[0] = 96.9000015f;
    runtime.TranslateMatrix(state.second.first_matrix,
                            state.second.first_translation, 0u);
    runtime.TranslateMatrix(state.second.second_matrix,
                            state.second.second_translation, 0u);
    const auto pipeline_first_matrix =
        EnsureMatrix(state.second.first_matrix, runtime);
    const auto pipeline_second_matrix =
        state.second.second_matrix != 0u ? pipeline_first_matrix :
        EnsureMatrix(state.second.second_matrix, runtime);
    const SkyClumpPipeline second_pipeline{
        2u, second_texture_object, option_one, option_two,
        pipeline_first_matrix, pipeline_second_matrix};
    runtime.SetClumpPipeline(state.second.clump, second_pipeline);
    if (second_texture_object != 0u)
        runtime.DestroyTexture(second_texture_object);
    state.second.active = 1u;
    runtime.ClearVertexColor(state.second.clump, 32u);
    runtime.SetVertexColor(state.second.clump, 0xffffffffu);
    return true;
}

std::uint32_t SceneSkyDualClumps::UpdateMatrices(
    SceneSkyDualState& state, const float value,
    ISceneSkyDualRuntime& runtime) {
    if (state.first.active != 0u) {
        state.first.second_translation[0] = value * 0.0019f;
        runtime.TranslateMatrix(state.first.second_matrix,
                                state.first.second_translation, 2u);
        (void)runtime.UpdateMatrix(state.first.second_matrix);
    }
    if (state.second.active == 0u) return state.second.active;
    state.second.first_translation[0] = value * 0.0038999999f;
    runtime.TranslateMatrix(state.second.first_matrix,
                            state.second.first_translation, 2u);
    (void)runtime.UpdateMatrix(state.second.first_matrix);
    state.second.second_translation[0] = value * 0.0068999999f;
    runtime.TranslateMatrix(state.second.second_matrix,
                            state.second.second_translation, 2u);
    return runtime.UpdateMatrix(state.second.second_matrix);
}

}  // namespace pal4
