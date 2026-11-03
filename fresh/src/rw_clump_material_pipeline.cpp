#include "pal4/rw_clump_material_pipeline.h"

namespace pal4 {

bool RwClumpMaterialPipeline::SetNormalMap(
    const std::uint32_t clump, const std::uint32_t material,
    IRwClumpMaterialPipelineRuntime& runtime) {
    if (clump == 0 || material == 0) return false;
    (void)runtime.ForAllAtomics(
        clump, RwClumpMaterialCallback::SetNormalMap, material);
    const auto pipeline = runtime.NormalPipeline();
    (void)runtime.ForAllAtomics(
        clump, RwClumpMaterialCallback::SetPipeline, pipeline);
    return true;
}

bool RwClumpMaterialPipeline::SetNormalMapTextures(
    const std::uint32_t clump, const std::uint32_t material,
    IRwClumpMaterialPipelineRuntime& runtime) {
    if (clump == 0 || material == 0) return false;
    (void)runtime.ForAllAtomics(
        clump, RwClumpMaterialCallback::SetNormalMapTextures, material);
    return true;
}

bool RwClumpMaterialPipeline::Select(
    const std::uint32_t clump, const bool use_skin,
    const bool use_normal_map,
    IRwClumpMaterialPipelineRuntime& runtime) {
    if (clump == 0) return false;
    const auto pipeline = use_normal_map
                              ? runtime.NormalPipeline()
                              : use_skin ? runtime.SkinPipeline(1u)
                                          : runtime.DefaultPipeline();
    (void)runtime.ForAllAtomics(
        clump, RwClumpMaterialCallback::SetPipeline, pipeline);
    return true;
}

bool RwClumpMaterialPipeline::SetEnvironmentMap(
    const std::uint32_t clump, const std::uint32_t texture,
    IRwClumpMaterialPipelineRuntime& runtime) {
    if (clump == 0 || texture == 0) return false;
    (void)runtime.ForAllAtomics(
        clump, RwClumpMaterialCallback::SetEnvironmentMap, texture);
    const auto pipeline = runtime.MaterialEffectPipeline(1u);
    (void)runtime.ForAllAtomics(
        clump, RwClumpMaterialCallback::SetPipeline, pipeline);
    return true;
}

bool RwClumpMaterialPipeline::SetMaterialEffect(
    const std::uint32_t clump, const std::uint32_t material,
    IRwClumpMaterialPipelineRuntime& runtime) {
    if (clump == 0 || material == 0) return false;
    (void)runtime.ForAllAtomics(
        clump, RwClumpMaterialCallback::SetMaterialEffect, material);
    const auto pipeline = runtime.MaterialEffectPipeline(1u);
    (void)runtime.ForAllAtomics(
        clump, RwClumpMaterialCallback::SetPipeline, pipeline);
    return true;
}

std::uint32_t RwClumpMaterialPipeline::Reset(
    const std::uint32_t clump,
    IRwClumpMaterialPipelineRuntime& runtime) {
    if (clump == 0) return 0;
    (void)runtime.ForAllAtomics(
        clump, RwClumpMaterialCallback::ResetMaterialEffect, 0);
    return runtime.ForAllAtomics(
        clump, RwClumpMaterialCallback::SetPipeline,
        runtime.DefaultPipeline());
}

}  // namespace pal4
