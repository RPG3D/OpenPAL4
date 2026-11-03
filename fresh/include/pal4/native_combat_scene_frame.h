#pragma once
#include "pal4/combat_scene_frame.h"
namespace pal4 {
struct NativeCombatSystemImage;
class NativeGameArenaManagerOwner;
class IGameWorldFrameRuntime;
// Borrows complete519C50 services. The caller must provide every required
// scene phase; a world-only drawing helper is not a substitute.
class NativeCombatSceneFrame final : public ICombatSceneFrameRuntime {
public:
    NativeCombatSceneFrame(NativeCombatSystemImage&,NativeGameArenaManagerOwner&,IGameWorldFrameRuntime&);
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint32_t CombatSystem() override;
    bool IsCombatSystemInactive(std::uint32_t) override;
    std::uint32_t ArenaManager() override;
    void ProcessArenaFrame(std::uint32_t,std::uint32_t) override;
private:
    NativeCombatSystemImage& image_;
    NativeGameArenaManagerOwner& arena_;
    IGameWorldFrameRuntime& world_;
    bool drawing_{};
};
}
