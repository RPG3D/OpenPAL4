#pragma once

#include "pal4/native_effect_active_frame.h"
#include "pal4/native_effect_creation.h"
#include "pal4/native_effect_group_lifecycle.h"
#include "pal4/effect_system_render_state.h"
#include "pal4/effect_system.h"
#include "pal4/native_rw_geometry_processor.h"

namespace pal4 {
class NativeBaseObjectMemory;

// palGameArenaEffMgr's verified native owner callbacks and geometry services.
// The group and memory are borrowed. Bind active playback after constructing it,
// then disable/destroy the group before unbinding or destroying playback.
class NativeArenaEffectServices final : public INativeEffectCreationEvents,
    public INativeEffectPlaybackEvents, public INativeEffectGroupLifecycleServices,
    public IEffectSystemRenderQueueRuntime, public IEffectSystemClumpRuntime,
    private IRwGeometryQueueAllocator {
public:
    NativeArenaEffectServices(NativeBaseObjectMemory&, std::uint32_t group);
    ~NativeArenaEffectServices() override;
    NativeArenaEffectServices(const NativeArenaEffectServices&) = delete;
    NativeArenaEffectServices& operator=(const NativeArenaEffectServices&) = delete;
    void BindPlayback(NativeEffectActiveFrame&);
    void UnbindPlayback() noexcept;
    void BindArena(std::uint32_t arena);
    // ActorManager53E230 uses CombatSystem's embedded world and propagates the
    // real AddClump result; Arena522AF0 always returns true after its call.
    void BindCombatWorld(std::uint32_t world);
    void UnbindArena() noexcept;
    bool AddActiveObject(std::uint32_t owner,std::uint32_t clump);
    void Created(std::uint32_t, const EffectSystemHandle&,
        const std::array<float, 3>&, std::uint32_t) override;
    void Started(std::uint32_t, const EffectSystemHandle&, std::int32_t) override;
    void Finished(std::uint32_t, const EffectSystemHandle&, std::int32_t) override;
    void Removed(std::uint32_t, const EffectSystemHandle&, std::uint32_t) override;
    void CleanupEffects(std::uint32_t group) override;
    void InitializeGeometryData(std::uint32_t geometry) override;
    std::uint8_t ReadByte(std::uint32_t) override;
    std::uint32_t ReadWord(std::uint32_t) override;
    void SortAtomicGeometries(std::uint32_t geometry) override;
    std::uint8_t ProcessAtomics(std::uint32_t clump, std::uint32_t geometry) override;
    std::uint8_t ProcessAtomicsWrapper(std::uint32_t clump) override;
private:
    std::uint32_t AllocateQueueBytes(std::uint32_t bytes) override;
    void FreeQueueBytes(std::uint32_t address) override;
    void ResetNotification(std::uint32_t, const EffectSystemHandle&);
    NativeBaseObjectMemory& memory_;
    std::uint32_t group_;
    NativeEffectActiveFrame* active_{};
    std::uint32_t arena_{};
    bool combat_world_{};
};
}  // namespace pal4
