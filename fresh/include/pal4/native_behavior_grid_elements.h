#pragma once
#include "pal4/native_npc_manager_owner.h"
#include "pal4/native_behavior_helper_storage.h"
#include <functional>
#include <stdexcept>
#include <utility>

namespace pal4 {
// Deletion is the original helper lifecycle. Per-frame physics is mandatory
// and supplied by its scene owner; there is no fallback update implementation.
class NativeBehaviorGridElements final : public INativeNpcGridElements {
public:
    using UpdateCallback=std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)>;
    NativeBehaviorGridElements(NativeBehaviorHelperStorage& storage,UpdateCallback update)
        : storage_(storage),update_(std::move(update)) {
        if(!update_) throw std::invalid_argument("native helper grid update is not bound");
    }
    void Update(std::uint32_t helper,std::uint32_t context,std::uint32_t argument) override {
        update_(helper,context,argument);
    }
    void Release(std::uint32_t helper) override { (void)storage_.Delete(helper); }
private:
    NativeBehaviorHelperStorage& storage_;
    UpdateCallback update_;
};
}
