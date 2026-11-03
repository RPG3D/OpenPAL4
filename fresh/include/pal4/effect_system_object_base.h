#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct EffectSystemObjectBaseState final {
    std::array<std::uint32_t, 5u> words{};
};

static_assert(sizeof(EffectSystemObjectBaseState) == 20u);

class EffectSystemObjectBase final {
public:
    static constexpr std::uint32_t kVtable = 0x84b240u;

    // IDA 0x63dbe0: reset bytes +4..+11 and words +12/+16.
    [[nodiscard]] static EffectSystemObjectBaseState& Construct(
        EffectSystemObjectBaseState& state);
    // IDA 0x63dc30: destructor tail restores only the base vtable.
    static void RestoreVtable(EffectSystemObjectBaseState& state);
};

}  // namespace pal4
