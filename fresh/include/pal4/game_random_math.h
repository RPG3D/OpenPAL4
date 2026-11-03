#pragma once

#include <cstdint>

namespace pal4 {

class IGameRandomMathRuntime {
public:
    virtual ~IGameRandomMathRuntime() = default;
    [[nodiscard]] virtual std::int32_t FloatToSeed(float value) = 0;
    virtual void SetRandomSeed(std::int32_t seed) = 0;
    [[nodiscard]] virtual std::int32_t RandomInteger() = 0;
};

class GameRandomMathRuntimeStub : public IGameRandomMathRuntime {
public:
    std::int32_t FloatToSeed(float) override { return 0; }
    void SetRandomSeed(std::int32_t) override {}
    std::int32_t RandomInteger() override { return 0; }
};

class GameRandomMath final {
public:
    // IDA 0x668370 and 0x6683c0; x87 returns a floating-point value.
    [[nodiscard]] static double SignedUnit(float seed,
                                            IGameRandomMathRuntime& runtime);
    [[nodiscard]] static double InRange(float minimum, float maximum,
                                         float seed,
                                         IGameRandomMathRuntime& runtime);
};

}  // namespace pal4
