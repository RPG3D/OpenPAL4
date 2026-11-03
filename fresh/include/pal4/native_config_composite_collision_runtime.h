#pragma once

#include "pal4/config_composite_collision.h"

namespace pal4 {

class INativeCollisionEventCallbacks {
public:
    virtual ~INativeCollisionEventCallbacks() = default;
    virtual void Enter(std::uint32_t resource,
                       std::uint32_t previous_state) = 0;
    virtual void Leave(std::uint32_t resource) = 0;
};

// RenderWare-backed services for the reconstructed ConfigComposite collision path.
class NativeConfigCompositeCollisionRuntime final
    : public IConfigCompositeCollisionRuntime {
public:
    explicit NativeConfigCompositeCollisionRuntime(
        INativeCollisionEventCallbacks* events = nullptr) noexcept
        : events_(events) {}
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    bool CheckLineSegment(const ConfigCollisionPosition& origin,
                          std::uint32_t atomic, CollisionPoint3& movement,
                          CollisionPoint3* normal, float* distance,
                          std::uint32_t mode) override;
    bool ProcessWorldCollision(const CollisionSegment& segment,
                               std::uint32_t atomic,
                               ConfigCollisionScratch& scratch) override;
    void NormalizeMovement(CollisionPoint3& output,
                           const CollisionPoint3& movement) override;
    bool HasPatchMesh(std::uint32_t atomic) override;
    bool HasHierarchy(std::uint32_t atomic) override;
    bool HasPluginData(std::uint32_t atomic) override;
    void NotifyEnter(std::uint32_t callback, std::uint32_t previous_state,
                     ConfigCompositeState& state) override;
    void NotifyLeave(std::uint32_t callback,
                     ConfigCompositeState& state) override;
    CollisionPoint3 VertexWorldPosition(std::uint32_t vertex_reference,
                                        std::uint32_t transform) override;
    CollisionPoint3 TransformNormal(const CollisionPoint3& normal,
                                    std::uint32_t transform) override;
    float VectorMagnitudeApprox(const CollisionPoint3& delta) override;
    std::uint32_t ObjectTransform(std::uint32_t frame) override;
    void ForAllAtomicIntersections(std::uint32_t atomic,
                                   const ConfigCollisionSphereQuery& query,
                                   std::uint32_t callback,
                                   ConfigCollisionClosestPoint& closest) override;
    std::uint32_t ProcessClumpCollision(const CollisionSegment& segment,
                                        std::uint32_t clump,
                                        ConfigCollisionScratch& scratch) override;

private:
    INativeCollisionEventCallbacks* events_ = nullptr;
};

}  // namespace pal4
