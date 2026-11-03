#include "pal4/rw_atomic_material_callbacks.h"

#include <bit>

namespace pal4 {

namespace {

float ReadFloat(IRwAtomicMaterialRuntime& runtime,
                const std::uint32_t address) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

std::uint32_t CreateNormalTexture(
    const std::uint32_t material, const std::uint32_t scale_address,
    IRwAtomicMaterialRuntime& runtime) {
    const auto source = runtime.ReadWord(material);
    const auto scale = ReadFloat(runtime, scale_address);
    return RwTextureImageFactory::CreateFromTexture(
        source, scale, runtime.TextureRuntime());
}

}  // namespace

std::uint32_t RwAtomicMaterialCallbacks::SetBasicMaterialTextures(
    const std::uint32_t material, const std::uint32_t config,
    IRwAtomicMaterialRuntime& runtime) {
    if (material == 0 || config == 0) return material;
    auto normal = runtime.ReadWord(config);
    if (normal == 0) {
        normal = CreateNormalTexture(material, config + 4u, runtime);
        runtime.WriteWord(config, normal);
    }
    if (normal != 0) {
        runtime.SetNormalMapTexture(material, normal);
        runtime.SetNormalEnvTexture(material, 0);
    }
    return material;
}

std::uint32_t RwAtomicMaterialCallbacks::ApplyBasicNormalMap(
    const std::uint32_t atomic, const std::uint32_t config,
    IRwAtomicMaterialRuntime& runtime) {
    if (atomic == 0 || config == 0) return atomic;
    if (runtime.ReadWord(config + 8u) != 0 &&
        runtime.SkinAtomicType(atomic) != 2u)
        runtime.SetSkinAtomicType(atomic, 2u);
    const auto geometry = runtime.ReadWord(atomic + 24u);
    if (geometry != 0)
        runtime.ForAllMaterials(
            geometry, RwMaterialCallback::NormalMapBasic, config);
    runtime.InitializeNormalMapAtomic(atomic);
    return atomic;
}

std::uint32_t RwAtomicMaterialCallbacks::SetExtendedMaterialTextures(
    const std::uint32_t material, const std::uint32_t config,
    IRwAtomicMaterialRuntime& runtime) {
    if (material == 0 || config == 0) return material;
    const auto source = runtime.ReadWord(config + 16u);
    auto normal = runtime.ReadWord(config + 24u);
    if (source != 0 && normal == 0) {
        normal = CreateNormalTexture(material, config + 20u, runtime);
        runtime.WriteWord(config + 24u, normal);
    }
    if (normal == 0) return material;
    runtime.SetNormalMapTexture(material, normal);
    const auto environment = runtime.ReadWord(config + 4u);
    if (environment != 0)
        runtime.SetNormalEnvTexture(material, environment);
    runtime.SetNormalEnvFrame(material, runtime.ReadWord(config));
    const auto coefficient = ReadFloat(runtime, config + 8u);
    if (coefficient != 0.0f)
        runtime.SetNormalEnvCoefficient(material, coefficient);
    runtime.ModulateNormalEnv(material, runtime.ReadWord(config + 12u));
    return material;
}

std::uint32_t RwAtomicMaterialCallbacks::ApplyExtendedNormalMap(
    const std::uint32_t atomic, const std::uint32_t config,
    IRwAtomicMaterialRuntime& runtime) {
    if (atomic == 0 || config == 0) return atomic;
    if (runtime.ReadWord(config + 28u) != 0 &&
        runtime.SkinAtomicType(atomic) != 2u)
        runtime.SetSkinAtomicType(atomic, 2u);
    const auto geometry = runtime.ReadWord(atomic + 24u);
    if (geometry != 0)
        runtime.ForAllMaterials(
            geometry, RwMaterialCallback::NormalMapExtended, config);
    runtime.InitializeNormalMapAtomic(atomic);
    return atomic;
}

std::uint32_t RwAtomicMaterialCallbacks::SetMatFxMaterial(
    const std::uint32_t material, const std::uint32_t config,
    IRwAtomicMaterialRuntime& runtime) {
    if (material == 0 || config == 0) return material;
    const auto mode = runtime.ReadWord(config);
    if (mode == 0u) {
        const auto texture = runtime.ReadWord(config + 4u);
        if (texture != 0) {
            runtime.SetMatFxEffects(material, 4u);
            runtime.SetupMatFxDualTexture(
                material, texture,
                runtime.ReadWord(config + 8u),
                runtime.ReadWord(config + 12u));
        }
    } else if (mode == 1u) {
        const auto matrix = runtime.ReadWord(config + 16u);
        if (matrix != 0) {
            runtime.SetMatFxEffects(material, 5u);
            runtime.SetMatFxUvTransforms(material, matrix, 0);
        }
    } else if (mode == 2u) {
        const auto texture = runtime.ReadWord(config + 4u);
        if (texture == 0) return material;
        const auto first = runtime.ReadWord(config + 16u);
        const auto second = runtime.ReadWord(config + 20u);
        if (first != 0 && second != 0) {
            runtime.SetMatFxEffects(material, 6u);
            runtime.SetupMatFxDualTexture(
                material, texture,
                runtime.ReadWord(config + 8u),
                runtime.ReadWord(config + 12u));
            runtime.SetMatFxUvTransforms(material, first, second);
        }
    }
    return material;
}

}  // namespace pal4
