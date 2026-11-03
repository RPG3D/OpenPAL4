#pragma once

#include "pal4/native_dialog_particle_effects.h"
#include <stdexcept>
#include <unordered_map>

namespace pal4 {
// A shared Dialog pool retains its playback services across owner changes.
// Route by the actual parent group, keeping each group's queue validation.
// All bound queues must outlive their binding and every Dialog using it.
class NativeEffectQueueRouter final : public INativeDialogParticleQueues {
public:
    void Bind(std::uint32_t owner, INativeDialogParticleQueues& queues) {
        if (!owner) throw std::invalid_argument("effect queue owner is null");
        const auto [it, inserted] = queues_.emplace(owner, &queues);
        if (!inserted && it->second != &queues)
            throw std::logic_error("effect queue owner already bound");
    }
    void Unbind(std::uint32_t owner) { queues_.erase(owner); }
    void QueueEffect(std::uint32_t parent, std::uint32_t effect) override {
        For(parent).QueueEffect(parent, effect);
    }
    void RemoveEffect(std::uint32_t parent, std::uint32_t effect) override {
        For(parent).RemoveEffect(parent, effect);
    }
private:
    INativeDialogParticleQueues& For(std::uint32_t parent) {
        const auto it = queues_.find(parent);
        if (it == queues_.end()) throw std::logic_error("effect parent queue is not bound");
        return *it->second;
    }
    std::unordered_map<std::uint32_t, INativeDialogParticleQueues*> queues_;
};
}  // namespace pal4
