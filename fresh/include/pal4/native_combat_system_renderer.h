#pragma once
#include <cstdint>
#include "pal4/combat_system_render.h"
#include <memory>
namespace pal4 {
struct NativeCombatSystemImage;
class NativeCombatActorManagerOwner;class NativeCombatWorldResources;
class NativeCombatPlayerSceneActors;class NativeCombatPlayerManager;
class NativeCombatMonsterManager;class NativeCombatConsole;
class NativeEffectSceneSession;class NativeCeguiUiSession;class NativeUiFrameUpdate;
class NativePalFontManager;class BackgroundMusicPlayer;class GameRenderState;
// Borrows actual battle/UI/debug owners. Camera Begin/End is the outer state's
// responsibility. This implements576220, not Entry/Start or the sound callback.
class NativeCombatSystemRenderer final : public ICombatSystemFrameRenderer {
public:
    NativeCombatSystemRenderer(NativeCombatSystemImage&,NativeCombatActorManagerOwner&,
        NativeCombatWorldResources&,NativeCombatPlayerSceneActors&,NativeCombatPlayerManager&,
        NativeCombatMonsterManager&,NativeCombatConsole&,NativeEffectSceneSession&,
        NativeCeguiUiSession&,NativeUiFrameUpdate&,NativePalFontManager&,
        BackgroundMusicPlayer&,GameRenderState&);
    ~NativeCombatSystemRenderer();
    NativeCombatSystemRenderer(const NativeCombatSystemRenderer&)=delete;
    NativeCombatSystemRenderer& operator=(const NativeCombatSystemRenderer&)=delete;
    void Render(std::uint32_t frame) override;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
