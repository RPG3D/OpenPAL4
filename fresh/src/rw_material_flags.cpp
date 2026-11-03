#include "pal4/rw_material_flags.h"

namespace pal4 {

std::uint32_t RwMaterialFlagsRuntimeStub::ForAllAtomics(
    std::uint32_t, RwAtomicFlagsCallback, std::uint32_t) {
    return 0u;
}

std::uint32_t RwMaterialFlagsRuntimeStub::ForAllWorldMaterials(
    std::uint32_t, RwMaterialFlagsCallback, std::uint32_t) {
    return 0u;
}

std::uint32_t RwMaterialFlagsRuntimeStub::ReadWord(std::uint32_t) {
    return 0u;
}

void RwMaterialFlagsRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}

std::uint32_t RwMaterialFlags::SetClump(
    const std::uint32_t clump, const std::uint32_t mode,
    IRwMaterialFlagsRuntime& runtime) {
    if (mode > 5u) return mode;
    return runtime.ForAllAtomics(clump, &SetAtomic, mode + 1u);
}

std::uint32_t RwMaterialFlags::SetAtomic(
    const std::uint32_t atomic, const std::uint32_t mode,
    IRwMaterialFlagsRuntime& runtime) {
    const auto geometry = runtime.ReadWord(atomic + 24u);
    if (geometry == 0u) return atomic;
    const auto count = static_cast<std::int32_t>(
        runtime.ReadWord(geometry + 36u));
    for (std::int32_t index = 0; index < count; ++index) {
        const auto materials = runtime.ReadWord(geometry + 32u);
        const auto material = runtime.ReadWord(
            materials + static_cast<std::uint32_t>(index) * 4u);
        (void)SetMaterial(material, mode, runtime);
    }
    return atomic;
}

std::uint32_t RwMaterialFlags::SetMaterial(
    const std::uint32_t material, const std::uint32_t mode,
    IRwMaterialFlagsRuntime& runtime) {
    const auto target = runtime.ReadWord(material);
    if (target != 0u) {
        runtime.WriteByte(target + 80u,
                          static_cast<std::uint8_t>(mode));
    }
    return material;
}

std::uint32_t RwMaterialFlags::SetWorld(
    const std::uint32_t world, const std::uint32_t mode,
    IRwMaterialFlagsRuntime& runtime) {
    if (mode > 5u) return mode;
    return runtime.ForAllWorldMaterials(world, &SetMaterial, mode + 1u);
}

}  // namespace pal4
