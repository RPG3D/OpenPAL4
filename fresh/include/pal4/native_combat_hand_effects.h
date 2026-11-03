#pragma once
#include <cstdint>
#include <memory>
#include <span>
struct RpClump;
namespace pal4 {
class NativeEffectSceneSession;
class NativePlayerSlotCopyRuntime;
class INativeSkillSystemServices;
// Borrows a real464-byte ActorInstance hand component, its clump and text owner.
// Retire this adapter before any borrowed storage/service; this never owns or
// loads the hand clump. The caller pins its resource owner during callbacks.
class NativeCombatHandEffects final {
public:
    NativeCombatHandEffects(std::span<std::uint32_t,116> component,RpClump&,
        NativePlayerSlotCopyRuntime&,NativeEffectSceneSession&,INativeSkillSystemServices&);
    ~NativeCombatHandEffects();
    NativeCombatHandEffects(const NativeCombatHandEffects&)=delete;
    NativeCombatHandEffects& operator=(const NativeCombatHandEffects&)=delete;
    void Refresh(); //529F90, including original redundant readiness queries.
    void SetActive(std::uint8_t,std::uint32_t context=0u); //52A2E0
    void Close(); //native retirement: remove handles even after partial refresh.
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
