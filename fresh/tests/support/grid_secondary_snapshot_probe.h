#pragma once

#include "pal4/grid_manager_members.h"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

// Deliberate synthetic memory fixture, not an original-game producer sample.
// Both native adapters must consume real member storage rather than return {}.
template<class Manager, class Check>
void VerifyNativeGridSecondarySnapshot(Manager& manager, Check check,
                                       bool use_primary_storage = false) {
    static_assert(sizeof(void*) == 4u);
    check(manager.GridObstacleCandidates().empty(), "fresh native secondary input is empty");
    auto& members = *reinterpret_cast<pal4::GridManagerMembersState*>(
        static_cast<std::uintptr_t>(manager.Address() + 0x38u));
    struct Restore {
        pal4::GridManagerMembersState& target;
        pal4::GridManagerMembersState saved;
        ~Restore() { target = saved; }
    } restore{members, members};
    std::array<std::uint32_t,4> storage{99u,11u,99u,0xbadu};
    const auto address = use_primary_storage ? members.elements_begin :
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(storage.data()));
    if (use_primary_storage)
        check(members.elements_end - members.elements_begin >= 12u,
              "owned secondary fixture needs three existing slots");
    const auto* slots = reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(address));
    const std::vector<std::uint32_t> expected{slots[0],slots[1],slots[2]};
    members.secondary_begin = address;
    members.secondary_end = members.secondary_begin + 12u;
    members.secondary_capacity = members.secondary_begin + 12u;
    const auto copied = manager.GridObstacleCandidates();
    check(copied == expected,
          "native adapter reads ordered live range without changing identities");
    members.secondary_end = members.secondary_begin;
    check(manager.GridObstacleCandidates().empty() && copied.size() == 3u,
          "native snapshot owns tokens independently of the cleared live range");
    members.secondary_capacity = members.secondary_begin + 2u;
    bool rejected = false;
    try { (void)manager.GridObstacleCandidates(); }
    catch (const std::logic_error&) { rejected = true; }
    check(rejected, "native adapter propagates corrupt secondary extent failure");
}
