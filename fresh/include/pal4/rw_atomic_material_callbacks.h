#pragma once

#include "pal4/rw_texture_image_factory.h"

#include <cstdint>

namespace pal4 {

enum class RwMaterialCallback {
    NormalMapBasic,
    NormalMapExtended,
    EnvironmentMap,
};

class IRwAtomicMaterialRuntime {
public:
    virtual ~IRwAtomicMaterialRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual IRwTextureImageFactoryRuntime& TextureRuntime() = 0;
    [[nodiscard]] virtual std::uint32_t SkinAtomicType(
        std::uint32_t atomic) = 0;
    virtual void SetSkinAtomicType(std::uint32_t atomic,
                                   std::uint32_t type) = 0;
    virtual void ForAllMaterials(std::uint32_t geometry,
                                 RwMaterialCallback callback,
                                 std::uint32_t config) = 0;
    virtual void InitializeNormalMapAtomic(std::uint32_t atomic) = 0;
    virtual void SetNormalMapTexture(std::uint32_t material,
                                     std::uint32_t texture) = 0;
    virtual void SetNormalEnvTexture(std::uint32_t material,
                                     std::uint32_t texture) = 0;
    virtual void SetNormalEnvFrame(std::uint32_t material,
                                   std::uint32_t frame) = 0;
    virtual void SetNormalEnvCoefficient(std::uint32_t material,
                                         float coefficient) = 0;
    virtual void ModulateNormalEnv(std::uint32_t material,
                                   std::uint32_t enabled) = 0;
    virtual void SetupMatFxAtomic(std::uint32_t atomic) = 0;
    virtual void SetMatFxEffects(std::uint32_t material,
                                 std::uint32_t mode) = 0;
    virtual void SetupMatFxEnvironment(
        std::uint32_t material, std::uint32_t texture,
        std::uint32_t frame, std::uint32_t unused,
        std::uint32_t coefficient_bits) = 0;
    virtual void SetupMatFxDualTexture(
        std::uint32_t material, std::uint32_t texture,
        std::uint32_t blend_mode, std::uint32_t flags) = 0;
    virtual void SetMatFxUvTransforms(
        std::uint32_t material, std::uint32_t first,
        std::uint32_t second) = 0;
};

class RwAtomicMaterialRuntimeStub : public IRwAtomicMaterialRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    IRwTextureImageFactoryRuntime& TextureRuntime() override {
        return texture_runtime_;
    }
    std::uint32_t SkinAtomicType(std::uint32_t) override { return 0; }
    void SetSkinAtomicType(std::uint32_t,
                           std::uint32_t) override {}
    void ForAllMaterials(std::uint32_t,
                         RwMaterialCallback,
                         std::uint32_t) override {}
    void InitializeNormalMapAtomic(std::uint32_t) override {}
    void SetNormalMapTexture(std::uint32_t,
                             std::uint32_t) override {}
    void SetNormalEnvTexture(std::uint32_t,
                             std::uint32_t) override {}
    void SetNormalEnvFrame(std::uint32_t,
                           std::uint32_t) override {}
    void SetNormalEnvCoefficient(std::uint32_t,
                                 float) override {}
    void ModulateNormalEnv(std::uint32_t,
                           std::uint32_t) override {}
    void SetupMatFxAtomic(std::uint32_t) override {}
    void SetMatFxEffects(std::uint32_t,
                         std::uint32_t) override {}
    void SetupMatFxEnvironment(
        std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t,
        std::uint32_t) override {}
    void SetupMatFxDualTexture(
        std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t) override {}
    void SetMatFxUvTransforms(
        std::uint32_t, std::uint32_t,
        std::uint32_t) override {}

private:
    RwTextureImageFactoryRuntimeStub texture_runtime_{};
};

class RwAtomicMaterialCallbacks final {
public:
    [[nodiscard]] static std::uint32_t SetPipeline(
        std::uint32_t atomic, std::uint32_t pipeline,
        IRwAtomicMaterialRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetBasicMaterialTextures(
        std::uint32_t material, std::uint32_t config,
        IRwAtomicMaterialRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyBasicNormalMap(
        std::uint32_t atomic, std::uint32_t config,
        IRwAtomicMaterialRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetExtendedMaterialTextures(
        std::uint32_t material, std::uint32_t config,
        IRwAtomicMaterialRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyExtendedNormalMap(
        std::uint32_t atomic, std::uint32_t config,
        IRwAtomicMaterialRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetEnvironmentMaterial(
        std::uint32_t material, std::uint32_t config,
        IRwAtomicMaterialRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyEnvironmentMap(
        std::uint32_t atomic, std::uint32_t config,
        IRwAtomicMaterialRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetMatFxMaterial(
        std::uint32_t material, std::uint32_t config,
        IRwAtomicMaterialRuntime& runtime);
};

}  // namespace pal4
