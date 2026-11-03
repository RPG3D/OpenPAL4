#include "pal4/atomic_pipeline_configurator.h"

namespace pal4 {

std::uint32_t AtomicPipelineConfigurator::SetAtomicPipeline(
    const std::uint32_t atomic, const std::uint32_t pipeline,
    IAtomicPipelineRuntime& runtime) {
    runtime.WriteWord(atomic + 108u, pipeline);
    return atomic;
}

std::uint32_t AtomicPipelineConfigurator::SetAtomicMaterialPipeline(
    const std::uint32_t atomic, const std::uint32_t pipeline,
    IAtomicPipelineRuntime& runtime) {
    runtime.SetupMaterialEffects(atomic);
    const auto geometry = runtime.ReadWord(atomic + 24u);
    if (geometry != 0u)
        (void)runtime.ForEachMaterial(geometry,
                                      MaterialPipelinePass::set_pipeline,
                                      pipeline);
    return atomic;
}

std::uint32_t AtomicPipelineConfigurator::SetClumpPipeline(
    const std::uint32_t clump, const std::uint32_t pipeline,
    IAtomicPipelineRuntime& runtime) {
    if (clump == 0u || pipeline == 0u) return 0u;
    (void)runtime.ForEachAtomic(clump,
                                AtomicPipelinePass::set_material_pipeline,
                                pipeline);
    const auto atomic_pipeline = runtime.GetD3D9Pipeline(1u);
    (void)runtime.ForEachAtomic(clump,
                                AtomicPipelinePass::set_atomic_pipeline,
                                atomic_pipeline);
    return 1u;
}

std::uint32_t AtomicPipelineConfigurator::ClearMaterialEffects(
    const std::uint32_t material, IAtomicPipelineRuntime& runtime) {
    runtime.SetMaterialEffects(material, 0u);
    return material;
}

std::uint32_t AtomicPipelineConfigurator::ClearAtomicMaterials(
    const std::uint32_t atomic, const std::uint32_t data,
    IAtomicPipelineRuntime& runtime) {
    const auto geometry = runtime.ReadWord(atomic + 24u);
    if (geometry != 0u)
        (void)runtime.ForEachMaterial(geometry,
                                      MaterialPipelinePass::clear_effects,
                                      data);
    return atomic;
}

std::uint32_t AtomicPipelineConfigurator::ClearClumpPipeline(
    const std::uint32_t clump, IAtomicPipelineRuntime& runtime) {
    if (clump == 0u) return 0u;
    (void)runtime.ForEachAtomic(clump,
                                AtomicPipelinePass::clear_material_effects,
                                0u);
    const auto offset = runtime.ReadWord(0x950cd0u);
    const auto base = runtime.ReadWord(0x979ef8u);
    const auto pipeline = runtime.ReadWord(base + offset + 60u);
    return runtime.ForEachAtomic(clump,
                                 AtomicPipelinePass::set_atomic_pipeline,
                                 pipeline);
}

}  // namespace pal4
