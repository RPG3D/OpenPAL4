#include "pal4/game_random_math.h"

#include <bit>

namespace pal4 {
namespace {

double Draw(const float seed, IGameRandomMathRuntime& runtime) {
    // FCOMP zero / FNSTSW / AND4100 / JNZ: only strictly positive seeds.
    if (seed > 0.0f)
        runtime.SetRandomSeed(runtime.FloatToSeed(seed));
    constexpr auto scale = std::bit_cast<double>(0x3f00002000400080ull);
    const volatile float unit = static_cast<float>(
        static_cast<double>(runtime.RandomInteger()) * scale);
    return unit;
}

}  // namespace

double GameRandomMath::SignedUnit(const float seed,
                                  IGameRandomMathRuntime& runtime) {
    const volatile float doubled = static_cast<float>(Draw(seed, runtime) * 2.0);
    const volatile float value = doubled - 1.0f;
    return value;
}

double GameRandomMath::InRange(const float minimum, const float maximum,
                               const float seed,
                               IGameRandomMathRuntime& runtime) {
    const auto unit = Draw(seed, runtime);
    const volatile float width = maximum - minimum;
    const volatile float scaled = static_cast<float>(unit * width);
    const volatile float value = scaled + minimum;
    return value;
}

}  // namespace pal4
