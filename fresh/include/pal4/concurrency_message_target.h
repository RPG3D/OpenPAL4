#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ConcurrencyMessageTargetState final {
    std::uint8_t unknown_prefix[88]{};
    std::uint8_t declining = 0u;
};
static_assert(offsetof(ConcurrencyMessageTargetState, declining) == 88u);

class ConcurrencyMessageTarget final {
public:
    // IDA 0x6578b0: mark incoming messages as declined.
    static void DeclineIncomingMessages(ConcurrencyMessageTargetState& state);
};

}  // namespace pal4
