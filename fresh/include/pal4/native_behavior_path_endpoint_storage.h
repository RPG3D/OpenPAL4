#pragma once
#include "pal4/native_behavior_helper_storage.h"
#include <utility>

namespace pal4 {
// Compatibility entry point for existing endpoint-only owners. All lifecycle
// work now belongs to the common five-class native helper storage.
class NativeBehaviorPathEndpointStorage final {
public:
    using DestroyAssociation = NativeBehaviorHelperStorage::DestroyAssociation;
    NativeBehaviorPathEndpointStorage(NativeBaseObjectMemory& memory,
        std::uint32_t& next_identifier, DestroyAssociation destroy)
        : storage_(memory, next_identifier, std::move(destroy)) {}
    void Construct(std::uint32_t helper) { storage_.Construct(0, helper); }
    void Destruct(std::uint32_t helper) { storage_.Destruct(0, helper); }
    static void SetScalar(BehaviorPathEndpointState& helper, std::uint32_t slot, float value) {
        NativeBehaviorHelperStorage::SetScalar(helper, slot, value);
    }
private:
    NativeBehaviorHelperStorage storage_;
};
}
