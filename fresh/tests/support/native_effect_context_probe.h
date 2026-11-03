#pragma once
#include "pal4/native_effect_handle_context_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_effect_tree_storage.h"
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace pal4::test {
// Native registry and tree storage are real. XML payload dispatch is deliberately
// a recording fixture: this probe does not prove Hook/Strip rendering integration.
inline void VerifyNativeEffectContextHandles(NativeBaseObjectMemory& memory,
    NativeEffectTreeStorage& trees, std::uint32_t group,
    IEffectHandlePropertyRuntime& properties) {
    const auto check=[](bool value) {
        if (!value) throw std::runtime_error("native context effect handle assertion");
    };
    const auto baseline=memory.OutstandingAllocations();
    struct Objects final : INativeContextEffectObjects {
        std::vector<std::uint32_t> stopped, destroyed, measured, updated, drawn;
        void Stop(std::uint32_t o) override { stopped.push_back(o); }
        void Destroy(std::uint32_t o, std::uint32_t flag) override {
            if (flag!=1u || stopped.empty() || stopped.back()!=o)
                throw std::runtime_error("context stop must precede deleting destructor");
            destroyed.push_back(o);
        }
        float DrainTime(std::uint32_t o) override { measured.push_back(o); return 0.5f; }
        void Update(std::uint32_t o, double seconds) override {
            if (seconds!=0.25) throw std::runtime_error("context timer delta");
            updated.push_back(o);
        }
        void Draw(std::uint32_t o, std::uint32_t c) override {
            if (c!=123u) throw std::runtime_error("context draw argument");
            drawn.push_back(o);
        }
    } objects;
    NativeEffectHandleContextRuntime runtime(memory,trees,group,properties,objects);
    const auto head=memory.ReadDword(group+232u);
    constexpr std::array<std::uint32_t,7> ids{70u,10u,90u,30u,50u,20u,80u};
    for (const auto id:ids) check(trees.InsertContext(group+228u,id,id+1000u).inserted);
    check(memory.ReadDword(group+240u)==ids.size());
    const auto duplicate=trees.InsertContext(group+228u,50u,9999u);
    check(!duplicate.inserted && memory.ReadDword(duplicate.node+16u)==1050u);
    // Exact matching must reject the lower bound at the next greater key.
    check(trees.LowerBoundContext(group+228u,31u)==duplicate.node);
    EffectSystemHandleWords absent{31u,group};
    check(!EffectHandle::IsReadyToShow(absent,runtime));
    EffectHandle::RemoveContext(absent,runtime);
    check(!absent.system && absent.id==0xffffffffu && objects.stopped.empty());
    check(trees.LowerBoundContext(group+228u,100u)==head);
    EffectSystemHandleWords invalid{0xffffffffu,group};
    check(!EffectHandle::IsReadyToShow(invalid,runtime));
    EffectHandle::DisplayAndCleanup(invalid,runtime);
    check(invalid.system==group); // negative ID returns before resetting
    EffectSystemHandleWords unregistered{50u,group+4u};
    check(!EffectHandle::IsReadyToShow(unregistered,runtime));
    EffectHandle::RemoveContext(unregistered,runtime);
    check(unregistered.system==group+4u);

    // Transfer, then expire, the payload. Erasing the ID node must not destroy
    // it early; the fifth tree owns its remaining lifetime after the transfer.
    EffectSystemHandleWords retiring{50u,group};
    check(EffectHandle::IsReadyToShow(retiring,runtime));
    EffectHandle::DisplayAndCleanup(retiring,runtime);
    check(!retiring.system && retiring.id==0xffffffffu &&
        memory.ReadDword(group+240u)==6u && memory.ReadDword(group+256u)==1u);
    check(objects.measured==std::vector<std::uint32_t>{1050u} && objects.destroyed.empty());
    check(!EffectSystem::InsertTimedEffect(group,1050u,9.0f,runtime));
    EffectSystem::DrawTimedEffects(group,123u,runtime);
    check(objects.drawn==std::vector<std::uint32_t>{1050u});
    EffectSystem::UpdateEffectTimers(group,0.25,runtime);
    check(memory.ReadDword(group+256u)==1u && objects.destroyed.empty());
    EffectSystem::UpdateEffectTimers(group,0.25,runtime);
    check(!memory.ReadDword(group+256u) &&
        objects.updated==std::vector<std::uint32_t>{1050u,1050u} &&
        objects.stopped==std::vector<std::uint32_t>{1050u} &&
        objects.destroyed==objects.stopped);
    // A stale copy must never retire the already destroyed object twice.
    retiring={50u,group};
    EffectHandle::DisplayAndCleanup(retiring,runtime);
    check(!retiring.system && objects.measured.size()==1u);
    for (const auto id:ids) {
        if (id==50u) continue;
        EffectSystemHandleWords h{id,group};
        check(EffectHandle::IsReadyToShow(h,runtime));
        EffectHandle::RemoveContext(h,runtime);
        check(!h.system && objects.destroyed.back()==id+1000u);
    }
    check(!memory.ReadDword(group+240u) && objects.stopped.size()==ids.size() &&
        objects.destroyed==objects.stopped && memory.OutstandingAllocations()==baseline);
    check(memory.ReadDword(head)==head && memory.ReadDword(head+8u)==head);
}
} // namespace pal4::test
