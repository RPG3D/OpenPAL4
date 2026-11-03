#pragma once
#include "pal4/behavior_path_endpoint_helper.h"
#include "pal4/object_extended_lifecycle.h"
#include "pal4/object_random_movement_lifecycle.h"
#include <functional>
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
// Construct/Destruct operate within the original extent and leave outer storage
// to the task owner. Delete implements the virtual deleting path and frees it.
class NativeBehaviorHelperStorage final : private IBehaviorPathEndpointLifecycleRuntime,
    private IObjectExtendedRuntime, private IObjectRandomMovementRuntime {
public:
    using DestroyAssociation = std::function<void(std::uint32_t, bool)>;
    using Random = std::function<std::uint32_t()>;
    NativeBehaviorHelperStorage(NativeBaseObjectMemory&, std::uint32_t& next_identifier,
        DestroyAssociation, Random random = {});
    ~NativeBehaviorHelperStorage() override;
    static std::uint32_t ImageSize(std::int32_t kind);
    void Construct(std::int32_t kind, std::uint32_t helper);
    void Destruct(std::int32_t kind, std::uint32_t helper);
    // Original shared virtual AC -> deleting destructor +98 with flags1.
    // Class0/1 share the same image and deleting lifecycle.
    std::uint32_t Delete(std::uint32_t helper);
    static void SetScalar(BehaviorPathEndpointState&, std::uint32_t slot, float value);
private:
    void ConstructBaseImage(void*);
    void CleanupBaseImage(void*);
    void ConstructBase(BehaviorPathEndpointState&) override;
    void DestructBase(BehaviorPathEndpointState&) override;
    void ConstructBase(ObjectMovementState&) override;
    void CleanupBase(ObjectMovementState&) override;
    void ConstructBase(ObjectRandomMovementState&) override;
    void CleanupBase(ObjectRandomMovementState&) override;
    BehaviorVector3 RandomVector() override;
    void Free(std::uint32_t) override;
    void CallCapturedSlotB0(std::uint32_t, std::uint32_t) override;
    std::uint32_t CallCurrentSlotA0(ObjectMovementState&) override;
    class GridRuntime;
    NativeBaseObjectMemory& memory_;
    Random random_;
    std::unique_ptr<GridRuntime> grid_;
};
}
