#include "pal4/game_random_math.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::GameRandomMathRuntimeStub {
public:
    std::int32_t FloatToSeed(float value) override {
        Check(value == expected_seed, "seed conversion input");
        events.push_back("convert"); return 42;
    }
    void SetRandomSeed(std::int32_t seed) override {
        Check(seed == 42, "seed conversion result");
        events.push_back("seed");
    }
    std::int32_t RandomInteger() override {
        events.push_back("rand"); return next_random;
    }
    float expected_seed{};
    std::int32_t next_random{};
    std::vector<std::string> events;
};

void SignedRange() {
    Runtime runtime;
    runtime.expected_seed = 3.5f;
    Check(pal4::GameRandomMath::SignedUnit(3.5f, runtime) == -1.0 &&
          runtime.events == std::vector<std::string>{
              "convert", "seed", "rand"},
          "positive seed precedes zero random draw");
    runtime.events.clear();
    runtime.next_random = 32767;
    Check(pal4::GameRandomMath::SignedUnit(-1.0f, runtime) == 1.0 &&
          runtime.events == std::vector<std::string>{"rand"},
          "negative seed preserves generator state and reaches upper bound");
}

void UserRange() {
    Runtime runtime;
    runtime.expected_seed = 0.0f;
    runtime.next_random = 0;
    Check(pal4::GameRandomMath::InRange(4.0f, 10.0f, 0.0f,
                                        runtime) == 4.0 &&
          runtime.events == std::vector<std::string>{"rand"},
          "zero seed preserves generator state and reaches lower range bound");
    runtime.events.clear();
    runtime.next_random = 32767;
    Check(pal4::GameRandomMath::InRange(4.0f, 10.0f, -1.0f,
                                        runtime) == 10.0 &&
          runtime.events == std::vector<std::string>{"rand"},
          "negative seed and upper range bound");
}

}  // namespace

int main() {
    SignedRange();
    UserRange();
}
