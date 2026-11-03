#pragma once

#include "pal4/collision_actor.h"

namespace pal4 {

class NativeArenaCollisionTree;
class NativeBaseObjectMemory;

// Routes a native CollisionActor path request through the owned arena tree.
class NativeCollisionActorPathRuntime final : public ICollisionActorPathRuntime {
public:
    NativeCollisionActorPathRuntime(NativeArenaCollisionTree& tree,
                                    NativeBaseObjectMemory& actor_memory)
        : tree_(tree), actor_memory_(actor_memory) {}

    [[nodiscard]] std::int32_t FaceCount(std::uint32_t tree) override;
    [[nodiscard]] std::int32_t RandomValue() override;
    [[nodiscard]] std::uint32_t FaceAt(std::uint32_t tree,
                                       std::uint32_t index) override;
    [[nodiscard]] D3dTriangleVector3 Centroid(std::uint32_t face) override;
    [[nodiscard]] std::uint32_t SelectFace(
        std::uint32_t tree, const D3dTriangleVector3& point) override;
    [[nodiscard]] D3dTriangleVector3 Project(
        std::uint32_t tree, std::uint32_t face,
        const D3dTriangleVector3& point) override;
    [[nodiscard]] bool TracePath(
        std::uint32_t tree, std::uint32_t path_address,
        std::uint32_t start_face, const D3dTriangleVector3& start,
        const D3dTriangleVector3& target, std::uint32_t target_face,
        std::uint32_t extra) override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;

private:
    void RequireTree(std::uint32_t tree) const;

    NativeArenaCollisionTree& tree_;
    NativeBaseObjectMemory& actor_memory_;
};

}  // namespace pal4
