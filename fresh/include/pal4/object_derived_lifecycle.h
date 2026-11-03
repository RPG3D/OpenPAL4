#pragma once

#include "pal4/object_extended_lifecycle.h"

namespace pal4 {

class ObjectDerivedLifecycle final {
public:
    static constexpr std::uint32_t kVtable = 0x842a30u;

    static ObjectMovementState& Construct(ObjectMovementState& state,
                                           IObjectExtendedRuntime& runtime);
    static void Cleanup(ObjectMovementState& state,
                        IObjectExtendedRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        ObjectMovementState& state, std::uint32_t object_address,
        std::uint32_t flags, IObjectExtendedRuntime& runtime);
    [[nodiscard]] static std::uint32_t Dispatch(
        ObjectMovementState& state, std::uint32_t first,
        std::uint32_t unused, IObjectExtendedRuntime& runtime);
};

}  // namespace pal4
