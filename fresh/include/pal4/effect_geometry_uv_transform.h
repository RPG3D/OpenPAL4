#pragma once

#include <cstdint>
#include <functional>

namespace pal4 {

class IEffectGeometryUvRuntime {
public:
    virtual ~IEffectGeometryUvRuntime() = default;
    virtual void ApplyToMaterials(std::uint32_t atomic,
                                  std::uint32_t first_matrix,
                                  std::uint32_t second_matrix) = 0;
};

class IEffectGeometryUvPipelineRuntime {
public:
    virtual ~IEffectGeometryUvPipelineRuntime() = default;
    virtual void SetupAtomic(std::uint32_t atomic) = 0;
    [[nodiscard]] virtual std::uint32_t AtomicGeometry(std::uint32_t atomic) = 0;
    [[nodiscard]] virtual std::uint32_t MaterialCount(
        std::uint32_t geometry) = 0;
    [[nodiscard]] virtual std::uint32_t ForAllMaterials(
        std::uint32_t geometry,
        const std::function<void(std::uint32_t)>& callback) = 0;
    virtual void SetEffects(std::uint32_t material,
                            std::uint32_t effect) = 0;
    virtual void SetUvMatrices(std::uint32_t material,
                               std::uint32_t first_matrix,
                               std::uint32_t second_matrix) = 0;
};

class EffectGeometryUvTransform final {
public:
    // IDA 0x64b4d0: material callback used by ApplyToMaterials.
    [[nodiscard]] static std::uint32_t ConfigureMaterial(
        std::uint32_t material, std::uint32_t effect,
        std::uint32_t first_matrix, std::uint32_t second_matrix,
        IEffectGeometryUvPipelineRuntime& runtime);
    // IDA 0x64b500: prepare atomic and visit its geometry materials.
    [[nodiscard]] static std::uint32_t ApplyToMaterials(
        std::uint32_t atomic, std::uint32_t first_matrix,
        std::uint32_t second_matrix,
        IEffectGeometryUvPipelineRuntime& runtime);
    // IDA 0x608eb0: first UV transform matrix.
    [[nodiscard]] static std::uint32_t ApplyFirst(
        std::uint32_t atomic, std::uint32_t matrix,
        IEffectGeometryUvRuntime& runtime);
    // IDA 0x608ed0: second UV transform matrix.
    [[nodiscard]] static std::uint32_t ApplySecond(
        std::uint32_t atomic, std::uint32_t matrix,
        IEffectGeometryUvRuntime& runtime);
};

}  // namespace pal4
