#include "pal4/rw_atomic_material_callbacks.h"

namespace pal4 {

std::uint32_t RwAtomicMaterialCallbacks::SetPipeline(
    const std::uint32_t atomic, const std::uint32_t pipeline,
    IRwAtomicMaterialRuntime& runtime) {
    if (atomic == 0) return 0;
    runtime.WriteWord(atomic + 108u, pipeline);
    return atomic;
}

std::uint32_t RwAtomicMaterialCallbacks::SetEnvironmentMaterial(
    const std::uint32_t material, const std::uint32_t config,
    IRwAtomicMaterialRuntime& runtime) {
    if (material == 0 || config == 0) return material;
    runtime.SetMatFxEffects(material, 2u);
    runtime.SetupMatFxEnvironment(
        material, runtime.ReadWord(config + 4u),
        runtime.ReadWord(config), 0u,
        runtime.ReadWord(config + 8u));
    return material;
}

std::uint32_t RwAtomicMaterialCallbacks::ApplyEnvironmentMap(
    const std::uint32_t atomic, const std::uint32_t config,
    IRwAtomicMaterialRuntime& runtime) {
    if (atomic == 0) return 0;
    runtime.SetupMatFxAtomic(atomic);
    const auto geometry = runtime.ReadWord(atomic + 24u);
    if (geometry != 0)
        runtime.ForAllMaterials(
            geometry, RwMaterialCallback::EnvironmentMap, config);
    return atomic;
}

}  // namespace pal4
