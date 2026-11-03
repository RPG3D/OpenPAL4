#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct D3dTriangleVector3 final {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

static_assert(sizeof(D3dTriangleVector3) == 12);

struct D3dTriangleVector2 final {
    float x = 0.0f;
    float z = 0.0f;
};

static_assert(sizeof(D3dTriangleVector2) == 8);

struct D3dTriangleFaceState final {
    std::array<std::uint32_t, 60u> words{};
};

static_assert(sizeof(D3dTriangleFaceState) == 240);

class ID3dTriangleEdgeQueryRuntime {
public:
    virtual ~ID3dTriangleEdgeQueryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Classify(
        D3dTriangleFaceState& state, std::uint32_t edge_offset,
        const D3dTriangleVector2& point, float threshold) = 0;
};

class D3dTriangleEdgeQueryRuntimeStub final
    : public ID3dTriangleEdgeQueryRuntime {
public:
    [[nodiscard]] std::uint32_t Classify(
        D3dTriangleFaceState&, std::uint32_t,
        const D3dTriangleVector2&, float) override { return 1u; }
};

struct D3dTriangleEdgeProbe final {
    D3dTriangleVector2 first{};
    D3dTriangleVector2 last{};
    D3dTriangleVector2 cached_normal{};
    std::array<std::uint8_t, 4u> flags{};
};

static_assert(sizeof(D3dTriangleEdgeProbe) == 28);

class ID3dTriangleTraceRuntime : public ID3dTriangleEdgeQueryRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Intersect(
        const D3dTriangleEdgeProbe& probe,
        D3dTriangleFaceState& state,
        std::uint32_t edge_offset,
        D3dTriangleVector2& intersection) = 0;
};

class ID3dTriangleNeighborRuntime {
public:
    virtual ~ID3dTriangleNeighborRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InstructionData(
        std::uint32_t instruction) = 0;
    virtual void UpdateNeighbor(std::uint32_t neighbor,
                                std::uint32_t instruction,
                                D3dTriangleFaceState& source,
                                float distance) = 0;
};

class ID3dTriangleStateRuntime {
public:
    virtual ~ID3dTriangleStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InstructionData(
        std::uint32_t instruction) = 0;
    [[nodiscard]] virtual D3dTriangleVector3 VectorData(
        std::uint32_t instruction) = 0;
    virtual void UpdateTransform(std::uint32_t instruction,
                                 D3dTriangleFaceState& state,
                                 bool incremental) = 0;
};

class D3dTriangleFace final {
public:
    // IDA 0x604260. The three edge destructors and face destructor are no-ops.
    static void Destroy(D3dTriangleFaceState& state);
    // IDA 0x601e80.
    static void Construct(D3dTriangleFaceState& state,
                          const D3dTriangleVector3& a,
                          const D3dTriangleVector3& b,
                          const D3dTriangleVector3& c);
    // IDA 0x601ef0.
    static void UpdateGeometry(D3dTriangleFaceState& state);
    // IDA 0x6020e0.
    [[nodiscard]] static std::uint8_t LinkNeighbor(
        D3dTriangleFaceState& state,
        const D3dTriangleVector3& first,
        const D3dTriangleVector3& last,
        std::uint32_t neighbor);
    // IDA 0x602200.
    [[nodiscard]] static std::uint32_t SetNeighbor(
        D3dTriangleFaceState& state, std::uint32_t edge,
        std::uint32_t neighbor);
    // IDA 0x602240/0x602280.
    [[nodiscard]] static bool IsOutsideEdges2D(
        D3dTriangleFaceState& state,
        const D3dTriangleVector2& point,
        ID3dTriangleEdgeQueryRuntime& runtime);
    [[nodiscard]] static bool IsOutsideEdges3D(
        D3dTriangleFaceState& state,
        const D3dTriangleVector3& point,
        ID3dTriangleEdgeQueryRuntime& runtime);
    // IDA 0x602320.
    [[nodiscard]] static std::uint32_t TraceEdges(
        D3dTriangleFaceState& state,
        const D3dTriangleEdgeProbe& probe,
        std::uint32_t& neighbor,
        std::uint32_t& edge_index,
        D3dTriangleVector2& intersection,
        ID3dTriangleTraceRuntime& runtime);
    // IDA 0x6023d0/0x6024e0/0x602590.
    [[nodiscard]] static D3dTriangleVector2& AdjustProbeAlongEdge(
        D3dTriangleFaceState& state, std::uint32_t edge,
        D3dTriangleEdgeProbe& probe,
        ID3dTriangleTraceRuntime& runtime);
    [[nodiscard]] static bool PushPointFromEdge(
        D3dTriangleFaceState& state, std::uint32_t edge,
        D3dTriangleVector2& point);
    [[nodiscard]] static bool ClampProbeToFace(
        D3dTriangleFaceState& state, D3dTriangleVector2& point,
        ID3dTriangleTraceRuntime& runtime);
    // IDA 0x6026b0/0x6026f0.
    [[nodiscard]] static bool ClampPointToFace(
        D3dTriangleFaceState& state, D3dTriangleVector3& point,
        ID3dTriangleTraceRuntime& runtime);
    [[nodiscard]] static bool PropagateNeighborState(
        D3dTriangleFaceState& state, std::uint32_t instruction,
        ID3dTriangleNeighborRuntime& runtime);
    // IDA 0x602770.
    [[nodiscard]] static bool UpdateState(
        D3dTriangleFaceState& state, std::uint32_t instruction,
        std::uint32_t source_neighbor, float distance,
        ID3dTriangleStateRuntime& runtime);
    // IDA 0x6028f0.
    static void UpdateVectorRadius(D3dTriangleFaceState& state,
                                   const D3dTriangleVector3& vector);
    // IDA 0x6022b0..0x602300.
    [[nodiscard]] static std::uint32_t VertexAddress(
        std::uint32_t state_address, std::uint32_t index);
    [[nodiscard]] static std::uint32_t CentroidAddress(
        std::uint32_t state_address);
    [[nodiscard]] static std::uint32_t Neighbor(
        const D3dTriangleFaceState& state, std::uint32_t edge);
    [[nodiscard]] static double WeightSum(
        const D3dTriangleFaceState& state);
    [[nodiscard]] static std::uint32_t Marker(
        const D3dTriangleFaceState& state);
    [[nodiscard]] static D3dTriangleVector3& CopyMidpoint(
        const D3dTriangleFaceState& state, std::uint32_t edge,
        D3dTriangleVector3& output);
    [[nodiscard]] static float FloatAt(const D3dTriangleFaceState& state,
                                       std::uint32_t byte_offset);
};

}  // namespace pal4
