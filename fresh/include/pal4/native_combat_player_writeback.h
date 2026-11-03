#pragma once
#include <cstdint>
namespace pal4 {
class NativeBaseObjectMemory;
class NativePlayerManagerSession;
class NativePlayerSlotProcessRuntime;
class NativePlayerScriptRefreshRuntime;
class NativeCombatPlayerActorOwner;
class NativeCombatPlayerManager;
// Borrows persistent Scene owners; no result storage belongs to Combat.
class NativeCombatPlayerWriteback final {
public:
    NativeCombatPlayerWriteback(NativePlayerManagerSession& players,NativeBaseObjectMemory& memory,
        NativePlayerSlotProcessRuntime& processor,NativePlayerScriptRefreshRuntime& refresh)
        :players_(players),memory_(memory),processor_(processor),refresh_(refresh){}
    std::uint32_t Copy(NativeCombatPlayerActorOwner&,std::uint32_t player_index);
    void Sync(NativeCombatPlayerManager&);
private:
    NativePlayerManagerSession& players_;
    NativeBaseObjectMemory& memory_;
    NativePlayerSlotProcessRuntime& processor_;
    NativePlayerScriptRefreshRuntime& refresh_;
};
}
