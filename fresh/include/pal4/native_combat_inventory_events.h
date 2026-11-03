#pragma once
#include <cstdint>
namespace pal4 {
class NativeInventorySession;
// Original inventory callbacks in the Combat event-handler table. The existing
// handler owner retains subscriptions; this adapter owns only a per-call snapshot.
class NativeCombatInventoryEvents final {
public:
    static std::uint32_t Handle(std::uint32_t callback,std::uint32_t payload,NativeInventorySession&);
};
}
