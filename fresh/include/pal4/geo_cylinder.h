#pragma once

#include "pal4/visual_model.h"

#include <cstdint>

namespace pal4 {

struct CylinderUv {
    float u{};
    float v{};
};

struct CylinderVertexPair {
    VisualModelVector first{};
    VisualModelVector second{};
    CylinderUv first_uv{};
    CylinderUv second_uv{};
};
static_assert(sizeof(CylinderVertexPair) == 40u);

struct CylinderCreateOptions {
    std::uint32_t texture{};
    int border_count{};
    float height{};
    float second_radius{};
    float first_radius{};
    bool bordered{};
    float border_width{};
    int arc_degrees{};
    bool alternate_uv{};
    bool centered{};
    std::uint8_t property{};
};

class IGeoCylinderRuntime {
public:
    virtual ~IGeoCylinderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t TextureWidth(
        std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::uint32_t TextureHeight(
        std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::uint32_t CreateGeometry(
        std::uint32_t vertices, std::uint32_t triangles,
        std::uint32_t flags) = 0;
    virtual void WriteVertex(std::uint32_t geometry, std::uint32_t index,
                             VisualModelVector position, CylinderUv uv,
                             std::uint32_t color) = 0;
    [[nodiscard]] virtual std::uint32_t CreateMaterial() = 0;
    virtual void SetMaterialTexture(std::uint32_t material,
                                    std::uint32_t texture) = 0;
    virtual void SetTriangle(std::uint32_t geometry, std::uint32_t index,
                             std::uint32_t first, std::uint32_t second,
                             std::uint32_t third,
                             std::uint32_t material) = 0;
    virtual void DestroyMaterial(std::uint32_t material) = 0;
    virtual void FinishGeometry(std::uint32_t geometry) = 0;
    [[nodiscard]] virtual std::uint32_t CreateAtomic() = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void SetAtomicFrame(std::uint32_t atomic,
                                std::uint32_t frame) = 0;
    virtual void SetAtomicGeometry(std::uint32_t atomic,
                                   std::uint32_t geometry) = 0;
    virtual void DestroyGeometry(std::uint32_t geometry) = 0;
    virtual void CaptureSharedAtomicFlags(std::uint32_t atomic) = 0;
    [[nodiscard]] virtual std::uint32_t CreateClump() = 0;
    virtual void SetClumpFrame(std::uint32_t clump,
                               std::uint32_t frame) = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void AddAtomic(std::uint32_t clump,
                           std::uint32_t atomic) = 0;
    virtual void AddChildFrame(std::uint32_t parent,
                               std::uint32_t child) = 0;
    virtual void SetMaterialFlagsForAllAtomics(
        std::uint32_t clump, std::uint32_t flags) = 0;
};

class GeoCylinder final {
public:
    // IDA 0x643a60.
    [[nodiscard]] static std::uint8_t Create(
        VisualModelState& model, const CylinderCreateOptions& options,
        IGeoCylinderRuntime& runtime);
};

}  // namespace pal4
