#include "pal4/effect_geometry_uv_transform.h"

namespace pal4 {

std::uint32_t EffectGeometryUvTransform::ConfigureMaterial(
    const std::uint32_t material, const std::uint32_t effect,
    const std::uint32_t first_matrix, const std::uint32_t second_matrix,
    IEffectGeometryUvPipelineRuntime& runtime) {
    runtime.SetEffects(material, effect);
    runtime.SetUvMatrices(material, first_matrix, second_matrix);
    return material;
}

std::uint32_t EffectGeometryUvTransform::ApplyToMaterials(
    const std::uint32_t atomic, const std::uint32_t first_matrix,
    const std::uint32_t second_matrix,
    IEffectGeometryUvPipelineRuntime& runtime) {
    runtime.SetupAtomic(atomic);
    const auto geometry = runtime.AtomicGeometry(atomic);
    const auto effect = 5u + (runtime.MaterialCount(geometry) > 1u ? 1u : 0u);
    return runtime.ForAllMaterials(geometry, [&](const std::uint32_t material) {
        (void)ConfigureMaterial(material, effect, first_matrix,
                                second_matrix, runtime);
    });
}

std::uint32_t EffectGeometryUvTransform::ApplyFirst(
    const std::uint32_t atomic, const std::uint32_t matrix,
    IEffectGeometryUvRuntime& runtime) {
    if (matrix == 0u)
        return 0u;
    runtime.ApplyToMaterials(atomic, matrix, 0u);
    return atomic;
}

std::uint32_t EffectGeometryUvTransform::ApplySecond(
    const std::uint32_t atomic, const std::uint32_t matrix,
    IEffectGeometryUvRuntime& runtime) {
    if (matrix == 0u)
        return 0u;
    runtime.ApplyToMaterials(atomic, 0u, matrix);
    return atomic;
}

}  // namespace pal4
