#pragma once
#include "pal4/native_effect_active_frame.h"
#include <memory>
#include <string_view>
namespace pal4 {
class NativeScriptEventRouter;
// ActorManager8452C8 effect slots12/20/24. Borrows the 400-byte manager,
// 520-byte CombatSystem image and synchronous router. Retire playback first.
class NativeCombatEffectNotifications final : public INativeEffectPlaybackEvents {
public:
    NativeCombatEffectNotifications(std::uint32_t manager,std::uint32_t combat,NativeScriptEventRouter&);
    ~NativeCombatEffectNotifications();
    void Started(std::uint32_t,const EffectSystemHandle&,std::int32_t) override;
    void Finished(std::uint32_t,const EffectSystemHandle&,std::int32_t) override;
    void Removed(std::uint32_t,const EffectSystemHandle&,std::uint32_t) override;
    void Message(std::uint32_t,const EffectSystemHandle&,std::string_view);
    [[nodiscard]] bool Busy() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
