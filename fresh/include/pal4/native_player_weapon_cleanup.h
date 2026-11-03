#pragma once

#include "pal4/base_object_named_weapon.h"

struct RpWorld;

namespace pal4 {
// Effect handles and named resource ownership belong to their existing owners.
class INativePlayerWeaponCleanupHost {
public:
    virtual ~INativePlayerWeaponCleanupHost() = default;
    virtual bool EffectNeedsUpdate(BaseObjectState&, std::uint32_t offset) = 0;
    virtual void UpdateEffect(BaseObjectState&, std::uint32_t offset) = 0;
    virtual bool EffectReady(BaseObjectState&, std::uint32_t offset) = 0;
    virtual void DisplayAndCleanupEffect(BaseObjectState&, std::uint32_t offset) = 0;
    virtual void CleanupNamedResource(BaseObjectState&, std::uint32_t resource_offset,
                                      std::uint32_t texture_context) = 0;
};

// Cleanup-only adapter. Borrows the current world and never destroys borrowed
// weapon clumps. Named clumps are released by the mandatory owning host.
class NativePlayerWeaponCleanup : public IBaseObjectNamedWeaponRuntime {
public:
    NativePlayerWeaponCleanup(RpWorld& world, std::uint32_t game,
                              INativePlayerWeaponCleanupHost& host)
        : world_(world), game_(game), host_(host) {}
    void Cleanup(BaseObjectState& state);
    std::uint32_t ReadDword(std::uint32_t address) override;
    std::uint32_t ArenaManager() override;
    std::uint32_t GameInstance() override;
    void RemoveClump(std::uint32_t arena, std::uint32_t clump) override;
    void RemoveFrameChild(std::uint32_t clump, std::string_view hook,
                           std::uint32_t frame) override;
    bool EffectNeedsUpdate(BaseObjectState&, std::uint32_t offset) override;
    void UpdateEffect(BaseObjectState&, std::uint32_t offset) override;
    bool EffectReady(BaseObjectState&, std::uint32_t offset) override;
    void DisplayAndCleanupEffect(BaseObjectState&, std::uint32_t offset) override;
    void CleanupClump(BaseObjectState&, std::uint32_t resource_offset,
                       std::uint32_t texture_context) override;
private:
    RpWorld& world_;
    std::uint32_t game_;
    INativePlayerWeaponCleanupHost& host_;
};
}  // namespace pal4
