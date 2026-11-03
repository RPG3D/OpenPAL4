#pragma once

#include "pal4/behavior_path_endpoint_movement.h"
#include "pal4/spatial_grid.h"

namespace pal4 {
class NativeBaseObjectMemory;

// Owns association allocations through memory; wrappers and their grids are
// borrowed. Destroy all associations before their wrapper/grid is reset.
// Helpers passed to Bind must be actual native images, never copied state.
class NativeBehaviorGridAssociationRuntime final :
    public ISpatialGridNodeRuntime, private IBehaviorPathAssociationRuntime {
public:
    explicit NativeBehaviorGridAssociationRuntime(NativeBaseObjectMemory& memory)
        : memory_(memory) {}
    [[nodiscard]] std::uint32_t Bind(BehaviorPathEndpointState& helper,
                                    std::uint32_t wrapper);
    [[nodiscard]] std::uint32_t UpdatePosition(std::uint32_t association,
                                               const BehaviorVector3& position);
    void DestroyAssociation(std::uint32_t association, bool free_self);

    std::uint32_t Allocate(std::uint32_t size) override;
    std::uint32_t Free(std::uint32_t address) override;
    SpatialGridState& GridAt(std::uint32_t address) override;
    SpatialGridNodeState& NodeAt(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t ReadWord(std::uint32_t address) override;

private:
    void ReleaseAssociation(std::uint32_t object, std::uint32_t flags) override;
    std::uint32_t BindAssociation(std::uint32_t binder,
                                 BehaviorPathEndpointState& helper) override;
    NativeBaseObjectMemory& memory_;
};
}  // namespace pal4
