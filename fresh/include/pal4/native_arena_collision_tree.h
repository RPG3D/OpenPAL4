#pragma once

#include "pal4/collision_tree_storage.h"
#include "pal4/collision_actor.h"
#include "pal4/game_arena_collision_tree.h"
#include "pal4/native_base_object_memory.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <unordered_map>

struct RpClump;
struct RpAtomic;

namespace pal4 {

class NativeGameArenaManagerOwner;

// Owns active and cached collision trees. Arena+404 and three cache records
// borrow these allocations; the arena text owner must outlive this owner.
class NativeArenaCollisionTree final : private IGameArenaCollisionRuntime,
                                       private ICollisionTreeAppendRuntime,
                                       private ICollisionTreeNeighborRuntime,
                                       public ICollisionActorPlacementRuntime {
public:
    NativeArenaCollisionTree() = default;
    NativeArenaCollisionTree(const NativeArenaCollisionTree&) = delete;
    NativeArenaCollisionTree& operator=(const NativeArenaCollisionTree&) = delete;
    ~NativeArenaCollisionTree() { Release(); }

    [[nodiscard]] bool Build(std::uint32_t arena, RpClump* floor);
    void BindCache(NativeGameArenaManagerOwner& owner);
    // IDA51BED0: retain an eligible tree in the original three-slot cache.
    void LeaveScene();
    // IDA51C060 plus the uncached active tree; final owner cleanup.
    void Release() noexcept;
    [[nodiscard]] std::size_t OwnedTreeCount() const noexcept { return trees_.size(); }

    [[nodiscard]] std::int32_t FaceCount() const noexcept;
    [[nodiscard]] std::uint32_t FaceAt(std::uint32_t index);
    [[nodiscard]] std::uint32_t SelectFace(
        const D3dTriangleVector3& point);
    [[nodiscard]] D3dTriangleVector3 ProjectToFace(
        std::uint32_t face, const D3dTriangleVector3& point);
    [[nodiscard]] std::uint32_t SnapPointToFace(
        D3dTriangleVector3& point);
    [[nodiscard]] bool TraceFacePath(
        std::uint32_t path_address, std::uint32_t start_face,
        const D3dTriangleVector3& start, std::uint32_t target_face,
        const D3dTriangleVector3& target,
        ID3dFacePathRuntime& path_memory);
    [[nodiscard]] std::uint32_t TraceFaceLine(
        std::uint32_t face, const D3dLine2State& line,
        std::uint32_t& neighbor);
    [[nodiscard]] std::uint32_t AdvanceAcrossFaces(
        const D3dTriangleVector3& start, std::uint32_t initial_face,
        D3dTriangleVector3& destination, std::uint32_t& final_face);
    void ClampToFace(std::uint32_t face, D3dTriangleVector3& point);
    [[nodiscard]] std::uint32_t TreeAddress() const noexcept { return tree_; }
    [[nodiscard]] std::uint32_t SelectFace(
        std::uint32_t tree, const D3dTriangleVector3& point) override;
    [[nodiscard]] D3dTriangleVector3 Project(
        std::uint32_t tree, std::uint32_t face,
        const D3dTriangleVector3& point) override;
    [[nodiscard]] std::uint32_t LinkedEdges() const noexcept;
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::string ReadCString(std::uint32_t address) override;
    [[nodiscard]] bool TextEqualsIgnoreCase(
        std::uint32_t, std::string_view) override;
    void InitializeText(std::uint32_t, std::string_view) override;
    void ReleaseObjectChildren(std::uint32_t) override;
    void DestroyObject(std::uint32_t) override;
    [[nodiscard]] std::uint32_t FreeObject(std::uint32_t) override;
    [[nodiscard]] std::uint32_t AllocateTree(std::uint32_t size) override;
    [[nodiscard]] std::uint32_t InitializeTree(std::uint32_t memory) override;
    void ForAllAtomics(std::uint32_t clump, std::uint32_t arena,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                  IGameArenaCollisionRuntime&)) override;
    void SetupRenderStates(std::uint32_t tree) override;
    void SetDebugRenderer(std::uint32_t tree) override;
    [[nodiscard]] std::uint16_t ReadHalfWord(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t GetObjectTransform(std::uint32_t frame) override;
    [[nodiscard]] CollisionRawVector TransformVertex(
        std::uint32_t vertex, std::uint32_t transform) override;
    [[nodiscard]] CollisionVector ConvertCoordinates(
        const CollisionRawVector& raw) override;
    [[nodiscard]] bool VectorsDiffer(
        const CollisionVector& left, const CollisionVector& right) override;
    void AddTriangle(std::uint32_t tree, const CollisionVector& first,
                     const CollisionVector& second,
                     const CollisionVector& third) override;
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t size) override;
    void ConstructFace(std::uint32_t address, const D3dTriangleVector3& first,
                       const D3dTriangleVector3& second,
                       const D3dTriangleVector3& third) override;
    void DestroyItem(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t Free(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ReadFacePointer(std::uint32_t slot) override;
    [[nodiscard]] D3dTriangleFaceState& Face(std::uint32_t address) override;

    static RpAtomic* VisitAtomic(RpAtomic* atomic, void* context) noexcept;

    NativeBaseObjectMemory memory_;
    NativeGameArenaManagerOwner* cache_owner_ = nullptr;
    std::uint32_t cache_arena_ = 0u;
    std::unordered_map<std::uint32_t,std::uint32_t> trees_; // tree -> linked edge count
    std::uint32_t arena_ = 0u;
    std::uint32_t tree_ = 0u;
    std::uint32_t linked_edges_ = 0u;
    std::uint32_t (*visitor_)(std::uint32_t, std::uint32_t,
                              IGameArenaCollisionRuntime&) = nullptr;
    std::exception_ptr visitor_error_;
    std::string last_error_;
};

}  // namespace pal4
