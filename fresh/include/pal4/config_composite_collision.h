#pragma once

#include "pal4/collision_segment_geometry.h"
#include "pal4/config_composite.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

using ConfigCollisionPosition = std::array<float, 4u>;
using ConfigCollisionScratch = std::array<std::uint32_t, 6u>;

struct ConfigCollisionTriangle final {
    CollisionPoint3 normal{};
    std::array<std::uint32_t, 4u> unknown{};
    std::array<std::uint32_t, 3u> vertex_references{};
};

static_assert(sizeof(ConfigCollisionTriangle) == 40);
static_assert(offsetof(ConfigCollisionTriangle, vertex_references) == 28);

struct ConfigCollisionClosestPoint final {
    CollisionPoint3 point{};
    float distance = 0.0f;
    std::uint32_t object_transform = 0u;
};

static_assert(sizeof(ConfigCollisionClosestPoint) == 20);

struct ConfigCollisionSphereQuery final {
    CollisionPoint3 center{};
    float radius = 0.0f;
    std::array<std::uint32_t, 2u> unused{};
    std::uint32_t type = 3u;
};

static_assert(sizeof(ConfigCollisionSphereQuery) == 28);

// Floor probes need only the clump segment boundary. Keep callers independent
// of unrelated wall, event and sphere traversal services.
class IConfigFloorClumpCollisionRuntime {
public:
    virtual ~IConfigFloorClumpCollisionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ProcessClumpCollision(
        const CollisionSegment& segment, std::uint32_t clump,
        ConfigCollisionScratch& scratch) = 0;
};
class IConfigCompositeCollisionRuntime:public IConfigFloorClumpCollisionRuntime {
public:
    virtual ~IConfigCompositeCollisionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual bool CheckLineSegment(
        const ConfigCollisionPosition& origin, std::uint32_t atomic,
        CollisionPoint3& movement, CollisionPoint3* normal,
        float* distance, std::uint32_t mode) = 0;
    [[nodiscard]] virtual bool ProcessWorldCollision(
        const CollisionSegment& segment, std::uint32_t atomic,
        ConfigCollisionScratch& scratch) = 0;
    virtual void NormalizeMovement(CollisionPoint3& output,
                                   const CollisionPoint3& movement) = 0;
    [[nodiscard]] virtual bool HasPatchMesh(std::uint32_t atomic) = 0;
    [[nodiscard]] virtual bool HasHierarchy(std::uint32_t atomic) = 0;
    [[nodiscard]] virtual bool HasPluginData(std::uint32_t atomic) = 0;
    virtual void NotifyEnter(std::uint32_t callback,
                             std::uint32_t previous_state,
                             ConfigCompositeState& state) = 0;
    virtual void NotifyLeave(std::uint32_t callback,
                             ConfigCompositeState& state) = 0;
    [[nodiscard]] virtual CollisionPoint3 VertexWorldPosition(
        std::uint32_t vertex_reference, std::uint32_t transform) = 0;
    [[nodiscard]] virtual CollisionPoint3 TransformNormal(
        const CollisionPoint3& normal, std::uint32_t transform) = 0;
    [[nodiscard]] virtual float VectorMagnitudeApprox(
        const CollisionPoint3& delta) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectTransform(
        std::uint32_t frame) = 0;
    virtual void ForAllAtomicIntersections(
        std::uint32_t atomic, const ConfigCollisionSphereQuery& query,
        std::uint32_t callback, ConfigCollisionClosestPoint& closest) = 0;
};

class ConfigCompositeCollisionRuntimeStub final
    : public IConfigCompositeCollisionRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] bool CheckLineSegment(
        const ConfigCollisionPosition&, std::uint32_t, CollisionPoint3&,
        CollisionPoint3*, float*, std::uint32_t) override { return false; }
    [[nodiscard]] bool ProcessWorldCollision(
        const CollisionSegment&, std::uint32_t,
        ConfigCollisionScratch&) override { return false; }
    void NormalizeMovement(CollisionPoint3& output,
                           const CollisionPoint3&) override { output.fill(0.0f); }
    [[nodiscard]] bool HasPatchMesh(std::uint32_t) override { return false; }
    [[nodiscard]] bool HasHierarchy(std::uint32_t) override { return false; }
    [[nodiscard]] bool HasPluginData(std::uint32_t) override { return false; }
    void NotifyEnter(std::uint32_t, std::uint32_t,
                     ConfigCompositeState&) override {}
    void NotifyLeave(std::uint32_t, ConfigCompositeState&) override {}
    [[nodiscard]] CollisionPoint3 VertexWorldPosition(
        std::uint32_t, std::uint32_t) override { return {}; }
    [[nodiscard]] CollisionPoint3 TransformNormal(
        const CollisionPoint3&, std::uint32_t) override { return {}; }
    [[nodiscard]] float VectorMagnitudeApprox(
        const CollisionPoint3&) override { return 0.0f; }
    [[nodiscard]] std::uint32_t ObjectTransform(
        std::uint32_t) override { return 0u; }
    void ForAllAtomicIntersections(
        std::uint32_t, const ConfigCollisionSphereQuery&,
        std::uint32_t, ConfigCollisionClosestPoint&) override {}
    [[nodiscard]] std::uint32_t ProcessClumpCollision(
        const CollisionSegment&, std::uint32_t,
        ConfigCollisionScratch&) override { return 0u; }
};

class ConfigCompositeCollision final {
public:
    // IDA 0x5ff240.
    [[nodiscard]] static bool CheckSecondList(
        const ConfigCompositeState& state,
        const ConfigCollisionPosition* start,
        ConfigCollisionPosition* position,
        CollisionPoint3& movement, std::uint32_t unused_word,
        CollisionPoint3* normal, float* distance,
        IConfigCompositeCollisionRuntime& runtime);

    // IDA 0x5ff3d0.
    [[nodiscard]] static bool CheckThirdList(
        const ConfigCompositeState& state,
        const ConfigCollisionPosition* start,
        const ConfigCollisionPosition* position,
        CollisionPoint3& movement, std::uint32_t& hard_blocked,
        std::uint32_t unused_first, std::uint32_t unused_second,
        IConfigCompositeCollisionRuntime& runtime);

    // IDA 0x5ff680.
    [[nodiscard]] static bool CheckFourthList(
        ConfigCompositeState& state,
        const ConfigCollisionPosition* start,
        const ConfigCollisionPosition* position,
        std::uint32_t excluded_type, CollisionPoint3& movement,
        std::uint32_t& hard_blocked,
        std::uint32_t unused_first, std::uint32_t unused_second,
        IConfigCompositeCollisionRuntime& runtime);

    // IDA 0x5ffcb0.
    [[nodiscard]] static const ConfigCollisionTriangle& RecordClosestTrianglePoint(
        const CollisionPoint3& query,
        const ConfigCollisionTriangle& triangle,
        std::uint32_t unused_intersection,
        ConfigCollisionClosestPoint& closest,
        IConfigCompositeCollisionRuntime& runtime);

    // IDA 0x5ffeb0.
    [[nodiscard]] static bool CheckLineSegmentIntersection(
        const ConfigCollisionPosition* start, std::uint32_t atomic,
        CollisionPoint3& movement, CollisionPoint3* normal,
        float* distance, std::uint32_t mode,
        IConfigCompositeCollisionRuntime& runtime);

    // IDA 0x600000: shared return tail, preserving EAX.
    [[nodiscard]] static std::uint32_t SharedIntersectionReturn(
        std::uint32_t result) noexcept;

    // IDA 0x600020.
    [[nodiscard]] static std::uint32_t CheckFloorClumpIntersection(
        const ConfigCompositeState& state, const CollisionPoint3& point,
        float upper_offset, float lower_offset,
        CollisionPoint3* adjusted_point, std::uint32_t* second_result,
        std::uint32_t* first_result,
        IConfigFloorClumpCollisionRuntime& runtime);
};

}  // namespace pal4
