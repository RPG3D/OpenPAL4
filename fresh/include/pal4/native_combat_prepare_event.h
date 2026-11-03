#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
struct NativeCombatSystemImage;
class NativeCombatSystemAccess;class NativeCombatEventHandlers;
class NativeCombatPlayerManager;class NativeCombatMonsterManager;class NativeCombatPlayerMotion;
class NativeCombatHints;class NativeGameDatabase;
// Actual57A770 over native stage cursors, virtual400 motion and ordinary hint.
// Borrowed owners must survive synchronous event dispatch and hint formatting.
class NativeCombatPrepareEvent final {
public:
    NativeCombatPrepareEvent(NativeCombatSystemImage&,NativeCombatSystemAccess&,NativeCombatEventHandlers&,
        NativeCombatPlayerManager&,NativeCombatMonsterManager&,NativeCombatPlayerMotion&,NativeCombatHints&,const NativeGameDatabase&);
    ~NativeCombatPrepareEvent();
    std::uint32_t Handle(std::uint32_t owner,std::uint32_t event);
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
