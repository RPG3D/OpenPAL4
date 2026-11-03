#pragma once
#include "pal4/native_combat_state_dispatch.h"
#include <functional>

namespace pal4 {
class NativeCombatStateOwner;
class ICombatSceneRuntime;
class ICombatSceneFrameRuntime;
class ICombatSystemRenderRuntime;
class ICombatSystemFrameRenderer;
class ICombatSceneUpdateRuntime;
class NativeCombatSceneUpdate;
class INativeCombatSceneLifecycle;

// Borrows the registered owner and all four real service boundaries. The
// application must unbind this dispatcher before destroying any dependency.
// This composes shared game algorithms; it does not implement missing services.
class NativeCombatStateRuntime final : public INativeCombatStateDispatch {
public:
    NativeCombatStateRuntime(NativeCombatStateOwner&, ICombatSceneRuntime&,
        ICombatSceneFrameRuntime&, ICombatSystemRenderRuntime&,
        ICombatSceneUpdateRuntime&, std::function<void()> validate);
    NativeCombatStateRuntime(NativeCombatStateOwner&,ICombatSceneRuntime&,
        ICombatSceneFrameRuntime&,ICombatSystemFrameRenderer&,
        ICombatSceneUpdateRuntime&,std::function<void()> validate);
    NativeCombatStateRuntime(NativeCombatStateOwner&,ICombatSceneRuntime&,
        ICombatSceneFrameRuntime&,ICombatSystemFrameRenderer&,
        NativeCombatSceneUpdate&,std::function<void()> validate);
    NativeCombatStateRuntime(NativeCombatStateOwner&,INativeCombatSceneLifecycle&,
        ICombatSceneFrameRuntime&,ICombatSystemFrameRenderer&,
        NativeCombatSceneUpdate&,std::function<void()> validate);
    std::uint32_t Address() const override;
    bool Enter(std::uint32_t previous_id, std::uint32_t document) override;
    void Leave(std::uint32_t next_id) override;
    void Render(std::uint32_t frame) override;
    std::uint32_t Update(float elapsed) override;
private:
    void RequireServices() const;
    NativeCombatStateOwner& owner_;
    INativeCombatSceneLifecycle* lifecycle_{};
    std::function<bool(std::uint32_t,std::uint32_t)> enter_;
    std::function<void(std::uint32_t)> leave_;
    std::function<void(std::uint32_t)> render_;
    std::function<std::uint32_t(float)> update_;
    std::function<void()> validate_;
};
}
