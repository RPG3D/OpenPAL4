#pragma once

#include "pal4/collision_actor.h"

namespace pal4 {

class NativeArenaCollisionTree;
class NativeBaseObjectMemory;

// Supplies owned path nodes and the scene's floor faces to actor movement.
class NativeCollisionActorMovementRuntime final
    : public ICollisionActorMovementRuntime {
public:
    NativeCollisionActorMovementRuntime(NativeArenaCollisionTree& tree,
                                        NativeBaseObjectMemory& memory)
        : tree_(tree), memory_(memory) {}

    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t size) override;
    void Free(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t Trace(
        std::uint32_t face, const D3dLine2State& line,
        std::uint32_t& neighbor) override;
    void ClampFacePoint(std::uint32_t face,
                        D3dTriangleVector3& point) override;
    [[nodiscard]] bool TestClearance(
        std::uint32_t callback, const D3dTriangleVector3& start,
        const D3dTriangleVector3& end, std::uint32_t event) override;
    [[nodiscard]] std::uint32_t AdvanceAcrossFaces(
        std::uint32_t tree, const D3dTriangleVector3& start,
        std::uint32_t face, D3dTriangleVector3& destination,
        std::uint32_t& final_face) override;

private:
    NativeArenaCollisionTree& tree_;
    NativeBaseObjectMemory& memory_;
};

}  // namespace pal4
