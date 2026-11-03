#pragma once
#include "pal4/native_combat_world_frame_services.h"
namespace pal4 {
class NativeCombatWorldResources;
class NativeCombatPlayerSceneActors;
class NativeCombatMonsterManager;

// Borrows both actual first-roster model services. Camera Begin/End belongs to
// the caller. Render keeps original53F740 world/transparent/component order.
class NativeCombatWorldFrame final : private INativeCombatWorldFrameServices {
public:
    NativeCombatWorldFrame(NativeCombatPlayerSceneActors&,NativeCombatMonsterManager&);
    NativeCombatWorldFrame(const NativeCombatWorldFrame&)=delete;
    NativeCombatWorldFrame& operator=(const NativeCombatWorldFrame&)=delete;
    void Render(NativeCombatWorldResources&,std::uint32_t camera);
private:
    std::uint32_t Address() const override;
    void ProcessActorUpdates(std::uint32_t camera) override;
    std::uint32_t UpdateActorComponents(std::uint32_t camera) override;
    NativeCombatPlayerSceneActors& players_;
    NativeCombatMonsterManager& monsters_;
    bool rendering_{};
};
}
