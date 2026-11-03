#pragma once
#include "pal4/game_arena_release.h"
#include "pal4/native_arena_object_release.h"
#include <memory>

namespace pal4 {
class NativeGameArenaManagerOwner;
class NativeEffectSceneSession;
class NativeArenaSceneResourceStage;

// A scoped composite of the creating owners. The gameplay adapter supplies
// script/camera/actor/player/pet services; it never owns middleware resources.
class NativeArenaReleaseRuntime final : public IGameArenaReleaseRuntime {
public:
    using Observer = NativeArenaObjectRelease::Observer;
    NativeArenaReleaseRuntime(NativeGameArenaManagerOwner&, NativeEffectSceneSession&,
        NativeArenaSceneResourceStage&, NativeArenaObjectReleaseContext,
        IGameArenaReleaseRuntime& gameplay, Observer observer = {});
    std::uint32_t ReadWord(std::uint32_t) override;
    void WriteWord(std::uint32_t, std::uint32_t) override;
    std::string_view ReadCString(std::uint32_t) override;
    void StoreText(std::uint32_t, std::string_view) override;
    std::uint32_t Call(GameArenaReleaseCall, std::uint32_t, std::uint32_t, std::uint32_t) override;
private:
    NativeGameArenaManagerOwner& arena_;
    NativeArenaSceneResourceStage& scene_;
    IGameArenaReleaseRuntime& gameplay_;
    Observer observer_;
    std::unique_ptr<IGameArenaReleaseRuntime> effects_, render_, environment_, objects_, world_;
};
} // namespace pal4
