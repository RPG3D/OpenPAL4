#pragma once

#include "pal4/behavior_path_endpoint_helper.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IBehaviorPathCollisionStateRuntime {
public:
    virtual ~IBehaviorPathCollisionStateRuntime() = default;
    [[nodiscard]] virtual BehaviorVector3 ReadPosition(
        std::uint32_t object, BehaviorContext3& scratch) = 0;
    [[nodiscard]] virtual float ReadPathRadius(
        std::uint32_t path_address) = 0;
};

class BehaviorPathCollisionStateRuntimeStub final
    : public IBehaviorPathCollisionStateRuntime {
public:
    BehaviorVector3 ReadPosition(
        std::uint32_t object, BehaviorContext3& scratch) override;
    float ReadPathRadius(std::uint32_t path_address) override;
};

class BehaviorPathCollisionState final {
public:
    static void UpdateFromObject(
        BehaviorPathEndpointState& state, std::uint32_t object,
        std::uint32_t ignored, IBehaviorPathCollisionStateRuntime& runtime);
    static void UpdateFromVector(
        BehaviorPathEndpointState& state, std::uint32_t ignored_one,
        std::uint32_t ignored_two, std::uint32_t ignored_three,
        const BehaviorVector3* position,
        IBehaviorPathCollisionStateRuntime& runtime);
    static void UpdateFromRecord(
        BehaviorPathEndpointState& state,
        const std::array<float, 5>* record, std::uint32_t ignored,
        IBehaviorPathCollisionStateRuntime& runtime);
};

}  // namespace pal4
