#pragma once

#include <cstdint>

namespace pal4 {

enum class RwClumpMaterialCallback {
    SetNormalMap,
    SetNormalMapTextures,
    SetPipeline,
    SetEnvironmentMap,
    SetMaterialEffect,
    ResetMaterialEffect,
};

class IRwClumpMaterialPipelineRuntime {
public:
    virtual ~IRwClumpMaterialPipelineRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ForAllAtomics(
        std::uint32_t clump, RwClumpMaterialCallback callback,
        std::uint32_t data) = 0;
    [[nodiscard]] virtual std::uint32_t NormalPipeline() = 0;
    [[nodiscard]] virtual std::uint32_t SkinPipeline(
        std::uint32_t type) = 0;
    [[nodiscard]] virtual std::uint32_t MaterialEffectPipeline(
        std::uint32_t type) = 0;
    [[nodiscard]] virtual std::uint32_t DefaultPipeline() = 0;
};

class RwClumpMaterialPipelineRuntimeStub final
    : public IRwClumpMaterialPipelineRuntime {
public:
    std::uint32_t ForAllAtomics(
        std::uint32_t, RwClumpMaterialCallback,
        std::uint32_t) override { return 0; }
    std::uint32_t NormalPipeline() override { return 0; }
    std::uint32_t SkinPipeline(std::uint32_t) override { return 0; }
    std::uint32_t MaterialEffectPipeline(
        std::uint32_t) override { return 0; }
    std::uint32_t DefaultPipeline() override { return 0; }
};

class RwClumpMaterialPipeline final {
public:
    [[nodiscard]] static bool SetNormalMap(
        std::uint32_t clump, std::uint32_t material,
        IRwClumpMaterialPipelineRuntime& runtime);
    [[nodiscard]] static bool SetNormalMapTextures(
        std::uint32_t clump, std::uint32_t material,
        IRwClumpMaterialPipelineRuntime& runtime);
    [[nodiscard]] static bool Select(
        std::uint32_t clump, bool use_skin,
        bool use_normal_map,
        IRwClumpMaterialPipelineRuntime& runtime);
    [[nodiscard]] static bool SetEnvironmentMap(
        std::uint32_t clump, std::uint32_t texture,
        IRwClumpMaterialPipelineRuntime& runtime);
    [[nodiscard]] static bool SetMaterialEffect(
        std::uint32_t clump, std::uint32_t material,
        IRwClumpMaterialPipelineRuntime& runtime);
    [[nodiscard]] static std::uint32_t Reset(
        std::uint32_t clump,
        IRwClumpMaterialPipelineRuntime& runtime);
};

}  // namespace pal4
