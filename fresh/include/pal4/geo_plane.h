#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct GeoPlaneVector {
    float x{};
    float y{};
    float z{};
};

struct GeoPlaneState {
    std::uint8_t active{};         // +0
    std::uint8_t reversed_vertices{}; // +1
    std::uint8_t rotation_first{};  // +2
    std::uint8_t option{};         // +3
    std::uint32_t geometry_state{}; // +4
    std::uint32_t clump{};          // +8
    std::uint32_t atomic{};         // +12
    std::uint32_t frame{};          // +16
    float width{};                 // +20
    float height{};                // +24
    std::uint32_t material_flags{}; // +28
    GeoPlaneVector scale{};      // +32
    GeoPlaneVector rotation{};   // +44
    GeoPlaneVector position{};   // +56
    std::array<std::uint32_t, 4u> vertex_bounds{}; // +68..+80
};
static_assert(sizeof(GeoPlaneState) == 84u);
static_assert(offsetof(GeoPlaneState, clump) == 8u);
static_assert(offsetof(GeoPlaneState, geometry_state) == 4u);
static_assert(offsetof(GeoPlaneState, atomic) == 12u);
static_assert(offsetof(GeoPlaneState, material_flags) == 28u);
static_assert(offsetof(GeoPlaneState, scale) == 32u);
static_assert(offsetof(GeoPlaneState, rotation) == 44u);
static_assert(offsetof(GeoPlaneState, position) == 56u);
static_assert(offsetof(GeoPlaneState, vertex_bounds) == 68u);

class IGeoPlaneRuntime {
public:
    virtual ~IGeoPlaneRuntime() = default;
    [[nodiscard]] virtual int SetMaterialFlagsForAllAtomics(
        std::uint32_t clump, std::uint32_t flags) = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void RotateFrame(std::uint32_t frame,
                             GeoPlaneVector axis, float angle,
                             std::uint32_t combine) = 0;
    virtual void ScaleFrame(std::uint32_t frame,
                            GeoPlaneVector scale,
                            std::uint32_t combine) = 0;
    virtual void TranslateFrame(std::uint32_t frame,
                                GeoPlaneVector position,
                                std::uint32_t combine) = 0;
    [[nodiscard]] virtual int UpdateFrameObjects(
        std::uint32_t frame) = 0;
};

class IGeoPlaneGeometryRuntime {
public:
    virtual ~IGeoPlaneGeometryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void LockGeometry(std::uint32_t geometry,
                              std::uint32_t flags) = 0;
    [[nodiscard]] virtual int UnlockGeometry(std::uint32_t geometry) = 0;
};

class IGeoPlaneDestroyRuntime {
public:
    virtual ~IGeoPlaneDestroyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ClumpWorld(
        std::uint32_t clump) = 0;
    virtual void RemoveClumpFromWorld(std::uint32_t world,
                                      std::uint32_t clump) = 0;
    virtual void ClearClumpFrame(std::uint32_t clump) = 0;
    virtual void DestroyClump(std::uint32_t clump) = 0;
    virtual void DestroyFrame(std::uint32_t frame) = 0;
};

class IGeoPlaneCreateRuntime : public IGeoPlaneRuntime,
                               public IGeoPlaneDestroyRuntime {
public:
    [[nodiscard]] virtual std::uint32_t CreateMaterial() = 0;
    virtual void SetMaterialTexture(std::uint32_t material,
                                    std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::uint32_t CreateGeometry(
        std::uint32_t vertices, std::uint32_t triangles,
        std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetTriangleIndices(std::uint32_t geometry,
                                    std::uint32_t triangle,
                                    std::array<std::uint32_t, 3u> indices) = 0;
    virtual void SetTriangleMaterial(std::uint32_t geometry,
                                     std::uint32_t triangle,
                                     std::uint32_t material) = 0;
    virtual void DestroyMaterial(std::uint32_t material) = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 4u>
        CalculateBoundingSphere(std::uint32_t morph_target) = 0;
    virtual void UnlockGeometry(std::uint32_t geometry) = 0;
    [[nodiscard]] virtual std::uint32_t CreateAtomic() = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void SetAtomicFrame(std::uint32_t atomic,
                                std::uint32_t frame) = 0;
    virtual void SetAtomicGeometry(std::uint32_t atomic,
                                   std::uint32_t geometry,
                                   std::uint32_t flags) = 0;
    virtual void DestroyGeometry(std::uint32_t geometry) = 0;
    [[nodiscard]] virtual std::uint32_t SharedAtomicCallback() = 0;
    virtual void SetSharedAtomicCallback(std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t CreateClump() = 0;
    virtual void AddAtomic(std::uint32_t clump,
                           std::uint32_t atomic) = 0;
    virtual void AddFrameChild(std::uint32_t parent,
                               std::uint32_t child) = 0;
};

class IGeoPlaneRenderRuntime {
public:
    virtual ~IGeoPlaneRenderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetRenderState(
        std::uint32_t state) = 0;
    virtual void SetRenderState(std::uint32_t state,
                                std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GetDeviceState(
        std::uint32_t state) = 0;
    virtual void SetDeviceState(std::uint32_t state,
                                std::uint32_t value) = 0;
    virtual void RenderClump(std::uint32_t clump) = 0;
    virtual void SetTextureStage(std::uint32_t first,
                                 std::uint32_t second,
                                 std::uint32_t texture,
                                 std::uint32_t stage) = 0;
    [[nodiscard]] virtual int ClearStreamSource(
        std::uint32_t stream) = 0;
};

class GeoPlaneObject final {
public:
    // IDA 0x6480a0.
    [[nodiscard]] static int SetMaterialFlags(
        GeoPlaneState& object, std::uint32_t flags,
        IGeoPlaneRuntime& runtime);
    // IDA 0x6480d0/0x648120/0x648170.
    [[nodiscard]] static int SetRotation(
        GeoPlaneState& object, GeoPlaneVector value,
        bool additive, IGeoPlaneRuntime& runtime);
    [[nodiscard]] static int SetScale(
        GeoPlaneState& object, GeoPlaneVector value,
        bool additive, IGeoPlaneRuntime& runtime);
    [[nodiscard]] static int SetPosition(
        GeoPlaneState& object, GeoPlaneVector value,
        bool additive, IGeoPlaneRuntime& runtime);
    // IDA 0x6481c0/0x648280.
    [[nodiscard]] static int UpdateGeometryVertices(
        GeoPlaneState& object,
        std::array<std::uint32_t, 4u> bounds,
        IGeoPlaneGeometryRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetVertexOrientation(
        GeoPlaneState& object, std::uint8_t reversed,
        IGeoPlaneGeometryRuntime& runtime);
    // IDA 0x648340.
    [[nodiscard]] static int SetGeometryMode(
        GeoPlaneState& object, int mode);
    // IDA 0x648370.
    [[nodiscard]] static std::uint8_t Create(
        GeoPlaneState& object, std::uint32_t texture,
        std::array<float, 2u> dimensions, std::uint8_t option,
        std::uint8_t plane_mode, IGeoPlaneCreateRuntime& runtime);
    // IDA 0x6486b0.
    static void Destroy(GeoPlaneState& object,
                        IGeoPlaneDestroyRuntime& runtime);
    // IDA 0x648710.
    [[nodiscard]] static int Render(
        const GeoPlaneState& object, std::uint32_t first,
        std::uint32_t second, IGeoPlaneRenderRuntime& runtime);
    // IDA 0x648870.
    [[nodiscard]] static int UpdateFrameTransform(
        const GeoPlaneState& object, IGeoPlaneRuntime& runtime);

private:
    [[nodiscard]] static int SetVector(
        GeoPlaneState& object, GeoPlaneVector& destination,
        GeoPlaneVector value, bool additive,
        IGeoPlaneRuntime& runtime);
    [[nodiscard]] static int WriteGeometryVertices(
        const GeoPlaneState& object,
        IGeoPlaneGeometryRuntime& runtime,
        bool conditional_order);
};

}  // namespace pal4
