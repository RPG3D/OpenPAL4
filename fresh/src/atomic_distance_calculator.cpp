#include "pal4/atomic_distance_calculator.h"

#include <bit>

namespace pal4 {

double AtomicDistanceCalculator::DistanceSquared(
    const std::uint32_t source_atomic,
    const std::uint32_t target_atomic,
    IAtomicDistanceRuntime& runtime) {
    if (source_atomic == 0u || target_atomic == 0u) return 0.0;
    if ((runtime.ReadWord(source_atomic + 76u) & 2u) != 0u)
        runtime.ResyncInterpolatedSphere(source_atomic);
    const auto source_transform = runtime.GetObjectTransform(
        runtime.ReadWord(source_atomic + 4u));
    AtomicPosition source{};
    runtime.CalculatePosition(source, source_atomic + 28u,
                              source_transform);
    const auto target_transform = runtime.GetObjectTransform(
        runtime.ReadWord(target_atomic + 4u));
    const auto x = static_cast<double>(source[0]) -
                   static_cast<double>(std::bit_cast<float>(
                       runtime.ReadWord(target_transform + 48u)));
    const auto y = static_cast<double>(source[1]) -
                   static_cast<double>(std::bit_cast<float>(
                       runtime.ReadWord(target_transform + 52u)));
    const auto z = static_cast<double>(source[2]) -
                   static_cast<double>(std::bit_cast<float>(
                       runtime.ReadWord(target_transform + 56u)));
    return y * y + x * x + z * z;
}

}  // namespace pal4
