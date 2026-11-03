#pragma once
#include "pal4/combat_system_render.h"
#include <functional>
#include <memory>
namespace pal4 {
struct NativeCombatSystemImage;
class NativeCombatConsole;class NativeCeguiUiSession;class NativeUiFrameUpdate;
// State0 has no battle models yet. Render its actual UI/Console through the
// shared576220 algorithm; active phases borrow the per-battle renderer.
class NativeCombatStateRenderer final : public ICombatSystemFrameRenderer {
public:
    NativeCombatStateRenderer(NativeCombatSystemImage&,NativeCombatConsole&,
        NativeCeguiUiSession&,NativeUiFrameUpdate&,
        std::function<ICombatSystemFrameRenderer&()> active);
    ~NativeCombatStateRenderer();
    void Render(std::uint32_t frame) override;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
