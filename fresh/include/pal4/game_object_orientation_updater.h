#pragma once

#include "pal4/behavior_path_endpoint_helper.h"
#include "pal4/game_object_lifecycle.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct GameObjectOrientationSubjectState {
    std::array<std::uint8_t, 0xbau> bytes{};
};

struct GameObjectOrientationGlobals {
    std::uint8_t guard{};
    float first_limit{};
    float second_limit{};
    BehaviorVector3 candidate{};
    BehaviorVector3 first_basis{};
    BehaviorVector3 second_basis{};
    BehaviorVector3 third_basis{};
};

class IGameObjectOrientationRuntime {
public:
    virtual ~IGameObjectOrientationRuntime() = default;
    [[nodiscard]] virtual double GetDoubleProperty(
        std::uint32_t manager_address) = 0;
    [[nodiscard]] virtual float ReadSubjectVectorComponent(
        std::uint32_t subject_address, std::uint32_t slot,
        std::uint32_t component) = 0;
    virtual void InitializeMovementAdapter() = 0;
    virtual void CheckObjectIntersection(
        BehaviorVector3& candidate, float first_limit,
        float second_limit, std::uint32_t option_a,
        std::uint32_t option_b) = 0;
    virtual void UpdateSubjectPosition(
        std::uint32_t subject_address,
        const BehaviorVector3& candidate) = 0;
    virtual void UpdateOrientation(
        std::uint32_t manager_address,
        const BehaviorVector3& first_basis,
        const BehaviorVector3& second_basis,
        const BehaviorVector3& third_basis,
        const BehaviorVector3& candidate) = 0;
};

class GameObjectOrientationRuntimeStub
    : public IGameObjectOrientationRuntime {
public:
    double GetDoubleProperty(std::uint32_t) override;
    float ReadSubjectVectorComponent(std::uint32_t, std::uint32_t,
                                     std::uint32_t) override;
    void InitializeMovementAdapter() override;
    void CheckObjectIntersection(BehaviorVector3&, float, float,
                                 std::uint32_t, std::uint32_t) override;
    void UpdateSubjectPosition(std::uint32_t,
                               const BehaviorVector3&) override;
    void UpdateOrientation(std::uint32_t, const BehaviorVector3&,
                           const BehaviorVector3&,
                           const BehaviorVector3&,
                           const BehaviorVector3&) override;
};

class GameObjectOrientationUpdater final {
public:
    // Shared target body; monster435D30/436730/437550 use1C/14/04.
    // Caller supplies the separately owned original per-target static cache.
    static void UpdateTarget(const GameObjectState& owner,
        const GameObjectOrientationSubjectState& subject, std::uint32_t subject_address,
        GameObjectOrientationGlobals& globals, IGameObjectOrientationRuntime& runtime,
        std::uint32_t manager_offset);
    // IDA 0x43a1f0. Subject byte +0xb9 gates collision and orientation.
    static void UpdateVtable3(
        const GameObjectState& owner,
        const GameObjectOrientationSubjectState& subject,
        std::uint32_t subject_address,
        GameObjectOrientationGlobals& globals,
        IGameObjectOrientationRuntime& runtime);
    // IDA 0x43abf0: same virtual path with manager at +0x1c.
    static void UpdateVtable4(
        const GameObjectState& owner,
        const GameObjectOrientationSubjectState& subject,
        std::uint32_t subject_address,
        GameObjectOrientationGlobals& globals,
        IGameObjectOrientationRuntime& runtime);
    // IDA 0x43b7e0: same virtual path with manager at +0x0c.
    static void UpdateVtable5(
        const GameObjectState& owner,
        const GameObjectOrientationSubjectState& subject,
        std::uint32_t subject_address,
        GameObjectOrientationGlobals& globals,
        IGameObjectOrientationRuntime& runtime);
    // IDA 0x43c0b0: same virtual path with manager at +0x04.
    static void UpdateVtable6(
        const GameObjectState& owner,
        const GameObjectOrientationSubjectState& subject,
        std::uint32_t subject_address,
        GameObjectOrientationGlobals& globals,
        IGameObjectOrientationRuntime& runtime);
};

}  // namespace pal4
