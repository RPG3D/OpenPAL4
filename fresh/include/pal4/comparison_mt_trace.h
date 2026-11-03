#pragma once
#include "pal4/comparison_random_call_trace.h"
#include <cstdlib>

namespace pal4 {
// MT state_before/state_after are raw draw indices, including refresh624->1.
// owner distinguishes native RNG instances; original uses its global state VA.
inline ComparisonRandomCallTrace& GetComparisonMtTrace() {
    static ComparisonRandomCallTrace trace(std::getenv("PAL4_COMPARE_SCRIPTED_INPUT") &&
        std::getenv("PAL4_COMPARE_MT_TRACE"),262144u,ComparisonRandomGenerator::mt);
    return trace;
}
} // namespace pal4
