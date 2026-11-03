#pragma once
#include <memory>
#include <array>
#include <cstdint>
namespace pal4 {
class NativeCombatWorldResources;
class NativeScriptEventRouter;
class INativeCombatSceneActors;
// Owns the original four mode3 scene subscriptions. All borrowed dependencies
// outlive it. Close clears queues/restores callbacks before releasing bindings.
class NativeCombatWorldSceneEvents final {
public:
    NativeCombatWorldSceneEvents(NativeCombatWorldResources&,NativeScriptEventRouter&,
        INativeCombatSceneActors& monsters,INativeCombatSceneActors* players=nullptr,
        std::array<std::uint32_t,8>* world_manager_slots=nullptr);
    ~NativeCombatWorldSceneEvents();
    NativeCombatWorldSceneEvents(const NativeCombatWorldSceneEvents&)=delete;
    NativeCombatWorldSceneEvents& operator=(const NativeCombatWorldSceneEvents&)=delete;
    void Close();
    std::uint32_t SubscriptionCount() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
