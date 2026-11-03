#pragma once

#include <cstdint>

namespace pal4 {

enum class AtomicPipelinePass { set_material_pipeline, set_atomic_pipeline,
                                clear_material_effects };
enum class MaterialPipelinePass { set_pipeline, clear_effects };

class IAtomicPipelineRuntime {
public:
    virtual ~IAtomicPipelineRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void SetupMaterialEffects(std::uint32_t atomic) = 0;
    [[nodiscard]] virtual std::uint32_t ForEachMaterial(
        std::uint32_t geometry, MaterialPipelinePass pass,
        std::uint32_t data) = 0;
    [[nodiscard]] virtual std::uint32_t ForEachAtomic(
        std::uint32_t clump, AtomicPipelinePass pass,
        std::uint32_t data) = 0;
    [[nodiscard]] virtual std::uint32_t GetD3D9Pipeline(
        std::uint32_t kind) = 0;
    virtual void SetMaterialEffects(std::uint32_t material,
                                    std::uint32_t effects) = 0;
};

class AtomicPipelineRuntimeStub final : public IAtomicPipelineRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void SetupMaterialEffects(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ForEachMaterial(
        std::uint32_t, MaterialPipelinePass, std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t ForEachAtomic(
        std::uint32_t, AtomicPipelinePass, std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t GetD3D9Pipeline(
        std::uint32_t) override { return 0u; }
    void SetMaterialEffects(std::uint32_t, std::uint32_t) override {}
};

class AtomicPipelineConfigurator final {
public:
    // IDA 0x5f9df0.
    [[nodiscard]] static std::uint32_t SetAtomicPipeline(
        std::uint32_t atomic, std::uint32_t pipeline,
        IAtomicPipelineRuntime& runtime);
    // IDA 0x5f9e00.
    [[nodiscard]] static std::uint32_t SetAtomicMaterialPipeline(
        std::uint32_t atomic, std::uint32_t pipeline,
        IAtomicPipelineRuntime& runtime);
    // IDA 0x5f9e30.
    [[nodiscard]] static std::uint32_t SetClumpPipeline(
        std::uint32_t clump, std::uint32_t pipeline,
        IAtomicPipelineRuntime& runtime);
    // IDA 0x5f9e70.
    [[nodiscard]] static std::uint32_t ClearMaterialEffects(
        std::uint32_t material, IAtomicPipelineRuntime& runtime);
    // IDA 0x5f9e90.
    [[nodiscard]] static std::uint32_t ClearAtomicMaterials(
        std::uint32_t atomic, std::uint32_t data,
        IAtomicPipelineRuntime& runtime);
    // IDA 0x5f9ec0.
    [[nodiscard]] static std::uint32_t ClearClumpPipeline(
        std::uint32_t clump, IAtomicPipelineRuntime& runtime);
};

}  // namespace pal4
