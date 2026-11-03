#include "pal4/combat_float_comparison_event.h"

#include <bit>
#include <cmath>

namespace pal4 {
namespace {

float ReadFloat(std::uint32_t address,
                ICombatFloatComparisonEventRuntime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

constexpr auto kTolerance = std::bit_cast<double>(0x3f1a36e2e0000000ull);

}  // namespace

void CombatFloatComparisonEvent::Process(
    std::uint32_t action, std::uint32_t target, std::uint32_t context,
    ICombatFloatComparisonEventRuntime& runtime) {
    const auto id = runtime.FloatToInt(ReadFloat(action + 20u, runtime));
    const auto event = runtime.LookupEvent(id);
    if (event == 0u) return;

    const auto value = runtime.ReadEventValue(event, context + 0x154u);
    const auto threshold = ReadFloat(action + 24u, runtime);
    const auto equal = std::fabs(std::fabs(value) -
                                 std::fabs(static_cast<double>(threshold))) <
                       kTolerance;
    const auto selected = ReadFloat(action + (equal ? 28u : 32u), runtime);
    if (selected == 0.0f) {
        if (equal) runtime.OnEqualZero(context, target);
        return;
    }
    runtime.Dispatch(target, runtime.FloatToInt(selected), context);
}

}  // namespace pal4
