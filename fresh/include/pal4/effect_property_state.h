#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct EffectPropertyState final {
    std::array<std::uint32_t, 20u> words{};
};
static_assert(sizeof(EffectPropertyState) == 80u);

class EffectPropertyLifecycle final {
public:
    // IDA 0x64bc10: clear three handles and set three floats to ten.
    [[nodiscard]] static EffectPropertyState& Reset(
        EffectPropertyState& state);
    // IDA 0x64bc30: clear only the three handles.
    static void Clear(EffectPropertyState& state);
};

}  // namespace pal4
