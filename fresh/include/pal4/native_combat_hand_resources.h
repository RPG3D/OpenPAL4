#pragma once
#include "pal4/actor_resource_loader.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
struct RpClump;
namespace pal4 {
class NativePkFileManagerRuntime;
class NativePlayerSlotCopyRuntime;
class NativeEffectSceneSession;
class INativeSkillSystemServices;
// Owns resources published into a borrowed real 464-byte ActorInstance hand.
// Component, parent body and services outlive this owner. Explicit Close may
// throw on effect callbacks and can be retried before retiring those services.
class NativeCombatHandResources final {
public:
    NativeCombatHandResources(std::span<std::uint32_t,116>,RpClump& parent,
        std::filesystem::path root,NativePkFileManagerRuntime&,
        NativePlayerSlotCopyRuntime&,NativeEffectSceneSession&,
        INativeSkillSystemServices&,std::function<bool()> inactive);
    ~NativeCombatHandResources();
    NativeCombatHandResources(const NativeCombatHandResources&)=delete;
    NativeCombatHandResources& operator=(const NativeCombatHandResources&)=delete;
    std::uint32_t Load(const ActorLoadRequest&);
    void SetActive(std::uint8_t,std::uint32_t context=0u);
    void Close();
    void Validate();
    [[nodiscard]] RpClump* Clump() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
