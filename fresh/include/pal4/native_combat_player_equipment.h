#pragma once
#include <cstdint>
namespace pal4 {
class NativeCombatPlayerActorOwner;
class NativeActorInstanceOwner;
class NativeInventorySession;
//539EA0 equipment selection on a real Player and its associated Combat model.
// Borrowed inventory and model services survive callbacks. Does not publish
// child808, register stage membership, or dispatch Player entry events.
class NativeCombatPlayerEquipment final {
public:
    static std::uint32_t Refresh(NativeCombatPlayerActorOwner&,
        NativeActorInstanceOwner&,NativeInventorySession&,std::int32_t version);
};
}
