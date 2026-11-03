#pragma once

#include "pal4/behavior_path_endpoint_helper.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct ObjectRandomMovementState {
    std::array<std::uint32_t, 56> words{};
};
static_assert(sizeof(ObjectRandomMovementState) == 0xe0u);

class IObjectRandomMovementRuntime {
public:
    virtual ~IObjectRandomMovementRuntime() = default;
    virtual void ConstructBase(ObjectRandomMovementState& state) = 0;
    virtual void CleanupBase(ObjectRandomMovementState& state) = 0;
    virtual void Free(std::uint32_t object_address) = 0;
    [[nodiscard]] virtual BehaviorVector3 RandomVector() = 0;
};

class ObjectRandomMovementRuntimeStub final
    : public IObjectRandomMovementRuntime {
public:
    void ConstructBase(ObjectRandomMovementState& state) override;
    void CleanupBase(ObjectRandomMovementState& state) override;
    void Free(std::uint32_t object_address) override;
    BehaviorVector3 RandomVector() override;
};

class ObjectRandomMovementLifecycle final {
public:
    static constexpr std::uint32_t kVtable = 0x842ae8u;

    static ObjectRandomMovementState& Construct(
        ObjectRandomMovementState& state,
        IObjectRandomMovementRuntime& runtime);
    static void Cleanup(ObjectRandomMovementState& state,
                        IObjectRandomMovementRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        ObjectRandomMovementState& state, std::uint32_t object_address,
        std::uint32_t flags, IObjectRandomMovementRuntime& runtime);
};

}  // namespace pal4
