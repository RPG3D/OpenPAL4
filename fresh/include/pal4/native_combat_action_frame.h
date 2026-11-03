#pragma once
#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCombatActionWindow;
class NativeCombatPlayerManager;
class NativeCombatSkillActorRegistry;
class NativeScriptEventRouter;
// Original541430 frame lifecycle and541570/5415A0 dialog consumers.
// Borrowed UI, roster, registry, router and tutorial service outlive Close.
class NativeCombatActionFrame final {
public:
    NativeCombatActionFrame(NativeCombatActionWindow&,NativeCombatPlayerManager&,
        NativeCombatSkillActorRegistry&,NativeScriptEventRouter&,std::uint32_t frame_manager,
        std::function<void(std::uint32_t)> tutorial);
    ~NativeCombatActionFrame();
    void SetVisible(bool,std::uint32_t parent=0u);
    std::size_t SubscriptionCount()const;
    std::size_t TutorialRequests()const noexcept;
    void Close();
private:
    class Impl;std::shared_ptr<Impl> impl_;
};
}
