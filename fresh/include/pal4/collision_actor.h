#pragma once

#include "pal4/d3d_face_path.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct CollisionActorState final {
    std::array<std::uint32_t, 40u> words{};
};

static_assert(sizeof(CollisionActorState) == 160);

class ICollisionActorPlacementRuntime {
public:
    virtual ~ICollisionActorPlacementRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SelectFace(
        std::uint32_t tree, const D3dTriangleVector3& point) = 0;
    [[nodiscard]] virtual D3dTriangleVector3 Project(
        std::uint32_t tree, std::uint32_t face,
        const D3dTriangleVector3& point) = 0;
};

class CollisionActorPlacementRuntimeStub final
    : public ICollisionActorPlacementRuntime {
public:
    [[nodiscard]] std::uint32_t SelectFace(
        std::uint32_t, const D3dTriangleVector3&) override { return 0u; }
    [[nodiscard]] D3dTriangleVector3 Project(
        std::uint32_t, std::uint32_t,
        const D3dTriangleVector3& point) override { return point; }
};

class ICollisionActorCallbackRuntime {
public:
    virtual ~ICollisionActorCallbackRuntime() = default;
    virtual void Output(std::uint32_t callback,
                        std::uint32_t destination_address,
                        std::uint32_t source_address,
                        std::uint32_t tick) = 0;
    virtual void Path(std::uint32_t callback,
                      std::uint32_t path_address,
                      std::uint32_t tick) = 0;
    [[nodiscard]] virtual std::uint32_t Move(
        std::uint32_t callback, std::uint32_t position_address,
        std::uint32_t point_address, std::uint32_t tick) = 0;
};

class CollisionActorCallbackRuntimeStub final
    : public ICollisionActorCallbackRuntime {
public:
    void Output(std::uint32_t, std::uint32_t,
                std::uint32_t, std::uint32_t) override {}
    void Path(std::uint32_t, std::uint32_t,
              std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Move(
        std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class ICollisionActorMovementRuntime : public ID3dFacePathRuntime,
                                       public ID3dFaceTraceReachabilityRuntime {
public:
    virtual void ClampFacePoint(std::uint32_t face,
                                D3dTriangleVector3& point) = 0;
    [[nodiscard]] virtual bool TestClearance(
        std::uint32_t callback, const D3dTriangleVector3& start,
        const D3dTriangleVector3& end, std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t AdvanceAcrossFaces(
        std::uint32_t tree, const D3dTriangleVector3& start,
        std::uint32_t face, D3dTriangleVector3& destination,
        std::uint32_t& final_face) = 0;
};

class CollisionActorMovementRuntimeStub final
    : public ICollisionActorMovementRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Allocate(
        std::uint32_t) override { return 0u; }
    void Free(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Trace(
        std::uint32_t, const D3dLine2State&,
        std::uint32_t&) override { return 0u; }
    void ClampFacePoint(std::uint32_t,
                        D3dTriangleVector3&) override {}
    [[nodiscard]] bool TestClearance(
        std::uint32_t, const D3dTriangleVector3&,
        const D3dTriangleVector3&,
        std::uint32_t) override { return false; }
    [[nodiscard]] std::uint32_t AdvanceAcrossFaces(
        std::uint32_t, const D3dTriangleVector3&,
        std::uint32_t, D3dTriangleVector3&,
        std::uint32_t&) override { return 1u; }
};

class ICollisionActorPathRuntime {
public:
    virtual ~ICollisionActorPathRuntime() = default;
    [[nodiscard]] virtual std::int32_t FaceCount(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::int32_t RandomValue() = 0;
    [[nodiscard]] virtual std::uint32_t FaceAt(
        std::uint32_t tree, std::uint32_t index) = 0;
    [[nodiscard]] virtual D3dTriangleVector3 Centroid(
        std::uint32_t face) = 0;
    [[nodiscard]] virtual std::uint32_t SelectFace(
        std::uint32_t tree, const D3dTriangleVector3& point) = 0;
    [[nodiscard]] virtual D3dTriangleVector3 Project(
        std::uint32_t tree, std::uint32_t face,
        const D3dTriangleVector3& point) = 0;
    [[nodiscard]] virtual bool TracePath(
        std::uint32_t tree, std::uint32_t path_address,
        std::uint32_t start_face, const D3dTriangleVector3& start,
        const D3dTriangleVector3& target, std::uint32_t target_face,
        std::uint32_t extra) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
};

class CollisionActorPathRuntimeStub final : public ICollisionActorPathRuntime {
public:
    [[nodiscard]] std::int32_t FaceCount(std::uint32_t) override { return 0; }
    [[nodiscard]] std::int32_t RandomValue() override { return 0; }
    [[nodiscard]] std::uint32_t FaceAt(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] D3dTriangleVector3 Centroid(
        std::uint32_t) override { return {}; }
    [[nodiscard]] std::uint32_t SelectFace(
        std::uint32_t, const D3dTriangleVector3&) override { return 0u; }
    [[nodiscard]] D3dTriangleVector3 Project(
        std::uint32_t, std::uint32_t,
        const D3dTriangleVector3& point) override { return point; }
    [[nodiscard]] bool TracePath(
        std::uint32_t, std::uint32_t, std::uint32_t,
        const D3dTriangleVector3&, const D3dTriangleVector3&,
        std::uint32_t, std::uint32_t) override { return false; }
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
};

class CollisionActor final {
public:
    static constexpr std::uint32_t kVtableAddress = 0x84a064u;

    // IDA 0x604580. Native stack byte at +80 is indeterminate.
    static CollisionActorState& Construct(
        CollisionActorState& state, std::uint8_t path_flag,
        ID3dFacePathRuntime& runtime);
    // IDA 0x604620.
    [[nodiscard]] static std::uint32_t Destroy(
        CollisionActorState& state, ID3dFacePathRuntime& runtime);
    // IDA 0x604680.
    [[nodiscard]] static std::uint32_t Place(
        CollisionActorState& state, std::uint32_t actor_address,
        std::uint32_t tree, std::uint32_t parameter_116,
        const D3dTriangleVector3& point, std::uint32_t face,
        std::uint32_t parameter_112,
        ICollisionActorPlacementRuntime& runtime);
    // IDA 0x604700.
    [[nodiscard]] static std::uint32_t ClearBinding(
        CollisionActorState& state);
    // IDA 0x604730..0x604760.
    [[nodiscard]] static std::uint32_t SetPathCallback(
        CollisionActorState& state, std::uint32_t callback);
    [[nodiscard]] static std::uint32_t SetMovementCallback(
        CollisionActorState& state, std::uint32_t callback);
    [[nodiscard]] static std::uint32_t SetOutputCallback(
        CollisionActorState& state, std::uint32_t callback);
    [[nodiscard]] static std::uint32_t SetCallback108(
        CollisionActorState& state, std::uint32_t callback);
    // IDA 0x604770.
    [[nodiscard]] static std::uint8_t DispatchCallbacks(
        const CollisionActorState& state, std::uint32_t actor_address,
        std::uint32_t tick, ICollisionActorCallbackRuntime& runtime);
    // IDA 0x6047d0.
    [[nodiscard]] static std::uint32_t UpdateMovement(
        CollisionActorState& state, std::uint32_t actor_address,
        std::uint32_t event, float elapsed,
        ICollisionActorMovementRuntime& runtime);
    // IDA 0x604a70.
    [[nodiscard]] static std::uint32_t StopMovement(
        CollisionActorState& state);
    // IDA 0x604a80/0x604ad0.
    [[nodiscard]] static bool RequestRandomPath(
        CollisionActorState& state, std::uint32_t actor_address,
        const D3dTriangleVector3& start, std::uint32_t extra,
        ICollisionActorPathRuntime& runtime);
    [[nodiscard]] static bool RequestPath(
        CollisionActorState& state, std::uint32_t actor_address,
        const D3dTriangleVector3& start,
        const D3dTriangleVector3& target,
        std::uint32_t target_face, std::uint32_t extra,
        ICollisionActorPathRuntime& runtime);
    // IDA 0x604b80.
    [[nodiscard]] static bool RedirectMovement(
        CollisionActorState& state, float step, std::uint32_t event,
        ICollisionActorMovementRuntime& runtime);
    // IDA 0x604d00..0x604d30.
    [[nodiscard]] static std::uint32_t AddVelocity(
        CollisionActorState& state, std::uint32_t actor_address,
        const D3dTriangleVector3& added);
    [[nodiscard]] static std::uint32_t SetSpeedBits(
        CollisionActorState& state, std::uint32_t speed_bits);
    [[nodiscard]] static std::uint8_t PathEnabled(
        const CollisionActorState& state);
    [[nodiscard]] static std::uint32_t PositionAddress(
        std::uint32_t actor_address);
};

}  // namespace pal4
