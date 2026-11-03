#pragma once

#include "pal4/d3d_face_traversal.h"
#include "pal4/d3d_line2.h"
#include "pal4/d3d_triangle_face.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct CollisionTreeStorageState final {
    std::array<std::uint32_t, 16> words{};
};

static_assert(sizeof(CollisionTreeStorageState) == 64);

class ICollisionTreeStorageRuntime {
public:
    virtual ~ICollisionTreeStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void DestroyItem(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class CollisionTreeStorageRuntimeStub final
    : public ICollisionTreeStorageRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    void DestroyItem(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Free(std::uint32_t) override { return 0; }
};

class ICollisionTreeAppendRuntime : public ICollisionTreeStorageRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void ConstructFace(std::uint32_t address,
                               const D3dTriangleVector3& first,
                               const D3dTriangleVector3& second,
                               const D3dTriangleVector3& third) = 0;
};

class ICollisionTreeFaceQueryRuntime {
public:
    virtual ~ICollisionTreeFaceQueryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadFacePointer(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool IsOutsideEdges3D(
        std::uint32_t face, const D3dTriangleVector3& point) = 0;
    virtual void ProjectHeight(std::uint32_t face,
                               D3dTriangleVector3& point) = 0;
    [[nodiscard]] virtual D3dTriangleVector3 Centroid(
        std::uint32_t face) = 0;
    [[nodiscard]] virtual std::uint32_t TraceToPoint(
        std::uint32_t face, const D3dTriangleVector2& from,
        const D3dTriangleVector2& to,
        D3dTriangleVector2& intersection) = 0;
};

class ICollisionTreePathRuntime : public ID3dFaceTraversalQueueRuntime {
public:
    virtual void InitializeSeedFace(std::uint32_t face,
                                    D3dFaceTraversalState& traversal) = 0;
    virtual void PropagateFace(std::uint32_t face,
                               D3dFaceTraversalState& traversal) = 0;
    [[nodiscard]] virtual std::uint32_t Marker(std::uint32_t face) = 0;
    [[nodiscard]] virtual D3dTriangleVector3 Midpoint(
        std::uint32_t face, std::uint32_t marker) = 0;
    [[nodiscard]] virtual D3dTriangleVector3 Project(
        std::uint32_t face, const D3dTriangleVector3& point) = 0;
    [[nodiscard]] virtual std::uint32_t Neighbor(
        std::uint32_t face, std::uint32_t marker) = 0;
    virtual void BeginActorPath(
        std::uint32_t actor, const CollisionTreeStorageState& storage,
        const D3dTriangleVector3& start, std::uint32_t start_face,
        const D3dTriangleVector3& target, std::uint32_t target_face) = 0;
    virtual void AddActorPoint(std::uint32_t actor,
                               const D3dTriangleVector3& point,
                               std::uint32_t face) = 0;
    virtual void EndActorPath(std::uint32_t actor) = 0;
};

class CollisionTreePathRuntimeStub final : public ICollisionTreePathRuntime {
public:
    [[nodiscard]] std::span<D3dFaceCandidate> Append(
        D3dFaceTraversalState&,
        const D3dFaceCandidate&) override { return {}; }
    [[nodiscard]] std::span<D3dFaceCandidate> Entries(
        D3dFaceTraversalState&) override { return {}; }
    void InitializeSeedFace(std::uint32_t,
                            D3dFaceTraversalState&) override {}
    void PropagateFace(std::uint32_t,
                       D3dFaceTraversalState&) override {}
    [[nodiscard]] std::uint32_t Marker(std::uint32_t) override { return 0; }
    [[nodiscard]] D3dTriangleVector3 Midpoint(
        std::uint32_t, std::uint32_t) override { return {}; }
    [[nodiscard]] D3dTriangleVector3 Project(
        std::uint32_t, const D3dTriangleVector3& point) override {
        return point;
    }
    [[nodiscard]] std::uint32_t Neighbor(
        std::uint32_t, std::uint32_t) override { return 0; }
    void BeginActorPath(std::uint32_t, const CollisionTreeStorageState&,
                        const D3dTriangleVector3&, std::uint32_t,
                        const D3dTriangleVector3&, std::uint32_t) override {}
    void AddActorPoint(std::uint32_t, const D3dTriangleVector3&,
                       std::uint32_t) override {}
    void EndActorPath(std::uint32_t) override {}
};

class ICollisionTreeMovementRuntime {
public:
    virtual ~ICollisionTreeMovementRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Trace(
        std::uint32_t face, const D3dLine2State& line,
        std::uint32_t& neighbor, std::uint32_t& edge,
        D3dTriangleVector2& intersection) = 0;
    virtual void AdjustEdge(std::uint32_t face, std::uint32_t edge,
                            D3dLine2State& line) = 0;
    virtual void ClampPoint(std::uint32_t face,
                            D3dTriangleVector2& point) = 0;
    virtual void ProjectHeight(std::uint32_t face,
                               D3dTriangleVector3& point) = 0;
};

class ICollisionTreeNeighborRuntime {
public:
    virtual ~ICollisionTreeNeighborRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadFacePointer(
        std::uint32_t slot) = 0;
    [[nodiscard]] virtual D3dTriangleFaceState& Face(
        std::uint32_t address) = 0;
};

class ICollisionTreePointProjectionRuntime {
public:
    virtual ~ICollisionTreePointProjectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SelectFace(
        std::uint32_t tree, const D3dTriangleVector3& point) = 0;
    [[nodiscard]] virtual D3dTriangleVector3 Project(
        std::uint32_t tree, std::uint32_t face,
        const D3dTriangleVector3& point) = 0;
    [[nodiscard]] virtual bool IsOutsideEdges(
        std::uint32_t face, const D3dTriangleVector3& point) = 0;
    virtual void ClampPoint(std::uint32_t face,
                            D3dTriangleVector3& point) = 0;
    virtual void ProjectHeight(std::uint32_t face,
                               D3dTriangleVector3& point) = 0;
};

class CollisionTreePointProjectionRuntimeStub final
    : public ICollisionTreePointProjectionRuntime {
public:
    [[nodiscard]] std::uint32_t SelectFace(
        std::uint32_t, const D3dTriangleVector3&) override { return 0u; }
    [[nodiscard]] D3dTriangleVector3 Project(
        std::uint32_t, std::uint32_t,
        const D3dTriangleVector3& point) override { return point; }
    [[nodiscard]] bool IsOutsideEdges(
        std::uint32_t, const D3dTriangleVector3&) override { return true; }
    void ClampPoint(std::uint32_t, D3dTriangleVector3&) override {}
    void ProjectHeight(std::uint32_t,
                       D3dTriangleVector3&) override {}
};

class CollisionTreeNeighborRuntimeStub final
    : public ICollisionTreeNeighborRuntime {
public:
    [[nodiscard]] std::uint32_t ReadFacePointer(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] D3dTriangleFaceState& Face(
        std::uint32_t) override { return face_; }

private:
    D3dTriangleFaceState face_{};
};

class CollisionTreeMovementRuntimeStub final
    : public ICollisionTreeMovementRuntime {
public:
    [[nodiscard]] std::uint32_t Trace(
        std::uint32_t, const D3dLine2State&,
        std::uint32_t&, std::uint32_t&,
        D3dTriangleVector2&) override { return 1u; }
    void AdjustEdge(std::uint32_t, std::uint32_t,
                    D3dLine2State&) override {}
    void ClampPoint(std::uint32_t,
                    D3dTriangleVector2&) override {}
    void ProjectHeight(std::uint32_t,
                       D3dTriangleVector3&) override {}
};

class CollisionTreeStorage final {
public:
    // IDA 0x603690. The native flag source is an uninitialized stack byte.
    static CollisionTreeStorageState& Initialize(
        CollisionTreeStorageState& state, std::uint8_t initial_flag);
    // IDA 0x6036f0.
    [[nodiscard]] static std::uint32_t Release(
        CollisionTreeStorageState& state,
        ICollisionTreeStorageRuntime& runtime);
    // IDA 0x603760.
    [[nodiscard]] static std::uint32_t ClearItems(
        CollisionTreeStorageState& state,
        ICollisionTreeStorageRuntime& runtime);
    // IDA 0x6037b0. Return is the new end address or old item count.
    [[nodiscard]] static std::uint32_t AppendTriangle(
        CollisionTreeStorageState& state,
        const D3dTriangleVector3& first,
        const D3dTriangleVector3& second,
        const D3dTriangleVector3& third,
        ICollisionTreeAppendRuntime& runtime);
    // IDA 0x6039b0.
    [[nodiscard]] static std::uint32_t SetDebugRenderer(
        CollisionTreeStorageState& state, std::uint32_t callback);
    // IDA 0x603a20.
    [[nodiscard]] static std::uint32_t SelectFace(
        const CollisionTreeStorageState& state,
        const D3dTriangleVector3& point,
        ICollisionTreeFaceQueryRuntime& runtime);
    // IDA 0x603be0.
    [[nodiscard]] static bool TraceFacePath(
        CollisionTreeStorageState& state, std::uint32_t actor,
        std::uint32_t start_face, const D3dTriangleVector3& start,
        std::uint32_t target_face, const D3dTriangleVector3& target,
        ICollisionTreePathRuntime& runtime);
    // IDA 0x603d40.
    [[nodiscard]] static std::uint32_t AdvanceAcrossFaces(
        const CollisionTreeStorageState& state,
        const D3dTriangleVector3& start, std::uint32_t initial_face,
        D3dTriangleVector3& destination, std::uint32_t& final_face,
        ICollisionTreeMovementRuntime& runtime);
    // IDA 0x604020. Returns the final pointer-array end address.
    [[nodiscard]] static std::uint32_t LinkFaceNeighbors(
        const CollisionTreeStorageState& state,
        ICollisionTreeNeighborRuntime& runtime);
    // IDA 0x604120/0x604140/0x604180.
    [[nodiscard]] static std::int32_t FaceCount(
        const CollisionTreeStorageState& state);
    [[nodiscard]] static std::uint32_t FaceAt(
        const CollisionTreeStorageState& state, std::uint32_t index,
        ICollisionTreeNeighborRuntime& runtime);
    [[noreturn]] static void ThrowInvalidFaceIndex();
    // IDA 0x604d40.
    [[nodiscard]] static std::uint32_t SnapPointToFace(
        std::uint32_t tree, D3dTriangleVector3& point,
        ICollisionTreePointProjectionRuntime& runtime);
};

}  // namespace pal4
