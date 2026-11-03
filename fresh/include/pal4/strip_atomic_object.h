#pragma once

#include <cstdint>
#include <vector>

namespace pal4 {

struct StripAtomicState {
    std::uint32_t atomic{};
};

struct StripPoint {
    float x{};
    float y{};
    float z{};
};

struct StripUv {
    float u{};
    float v{};
};

struct StripBoundingSphere {
    float x{};
    float y{};
    float z{};
    float radius{};
};

class IStripAtomicCreateRuntime {
public:
    virtual ~IStripAtomicCreateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreateMaterial() = 0;
    [[nodiscard]] virtual std::uint32_t CreateGeometry(
        std::uint32_t vertices, std::uint32_t triangles,
        std::uint32_t flags) = 0;
    virtual void WriteVertex(std::uint32_t geometry,
                             std::uint32_t index, StripPoint point,
                             StripUv uv) = 0;
    virtual void SetTriangle(std::uint32_t geometry,
                             std::uint32_t index, std::uint32_t first,
                             std::uint32_t second, std::uint32_t third,
                             std::uint32_t material) = 0;
    [[nodiscard]] virtual std::uint32_t CreateAtomic() = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void SetAtomicFrame(std::uint32_t atomic,
                                std::uint32_t frame) = 0;
    virtual void SetAtomicGeometry(std::uint32_t atomic,
                                   std::uint32_t geometry) = 0;
    virtual void CalculateVertexNormals(std::uint32_t geometry) = 0;
    [[nodiscard]] virtual StripBoundingSphere CalculateBoundingSphere(
        std::uint32_t geometry) = 0;
    virtual void SetBoundingSphere(std::uint32_t geometry,
                                   StripBoundingSphere sphere) = 0;
    virtual void UnlockGeometry(std::uint32_t geometry) = 0;
    virtual void DestroyGeometry(std::uint32_t geometry) = 0;
    [[nodiscard]] virtual std::uint32_t CachedOriginalCallback() = 0;
    [[nodiscard]] virtual std::uint32_t AtomicCallback(
        std::uint32_t atomic) = 0;
    virtual void CacheOriginalCallback(std::uint32_t callback) = 0;
    virtual void InstallCallback(std::uint32_t atomic, bool blended) = 0;
    virtual void InstallDefaultCallback(std::uint32_t atomic) = 0;
};

class IStripAtomicObjectRuntime {
public:
    virtual ~IStripAtomicObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GeometryOf(
        std::uint32_t atomic) = 0;
    [[nodiscard]] virtual std::uint32_t FirstMaterialOf(
        std::uint32_t geometry) = 0;
    [[nodiscard]] virtual std::uint32_t FrameOf(
        std::uint32_t atomic) = 0;
    virtual void DestroyMaterial(std::uint32_t material) = 0;
    virtual void DestroyFrame(std::uint32_t frame) = 0;
    virtual void SetAtomicFrame(std::uint32_t atomic,
                                std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyAtomic(
        std::uint32_t atomic) = 0;
};

class StripAtomicObject final {
public:
    // IDA 0x648f80.
    [[nodiscard]] static int AppendCylinderPath(
        std::vector<StripPoint>& points, int border_count,
        float height, float outer_radius, float inner_radius,
        int loops);

    // IDA 0x648f70.
    [[nodiscard]] static std::uint32_t GetFrame(
        const StripAtomicState& state,
        IStripAtomicObjectRuntime& runtime);

    // IDA 0x648ba0. Native callers discard the incidental EAX value.
    static void Create(StripAtomicState& state,
                       std::uint32_t material,
                       const std::vector<StripPoint>& points,
                       float height, bool alternate_uv, bool blended,
                       IStripAtomicCreateRuntime& runtime);

    // IDA 0x648b50.
    [[nodiscard]] static std::uint32_t Destroy(
        StripAtomicState& state, IStripAtomicObjectRuntime& runtime);
};

}  // namespace pal4
