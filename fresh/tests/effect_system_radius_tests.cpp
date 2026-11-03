#include "pal4/effect_system_radius.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemRadiusRuntime {
    std::uint8_t enabled = 0u;
    float squared = -1.0f;
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x2004u, "enabled offset");
        calls.push_back("read");
        return enabled;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x2004u, "enabled write offset");
        calls.push_back("enable");
        enabled = value;
    }
    void WriteFloat(std::uint32_t address, float value) override {
        Check(address == 0x2008u, "squared radius offset");
        calls.push_back("square");
        squared = value;
    }
    void CleanupEffects(std::uint32_t auxiliary) override {
        Check(auxiliary == 0x2000u, "cleanup owner");
        calls.push_back("cleanup");
    }
};

void TestInitialize() {
    Runtime runtime;
    Check(pal4::EffectSystemRadius::SetTimeAndFlag(
              0x2000u, -3.0f, runtime) == 1u &&
              runtime.enabled == 1u && runtime.squared == 9.0f &&
              runtime.calls ==
                  std::vector<std::string>{"cleanup", "enable", "square"},
          "initialization cleans effects before enabling squared radius");
}

void TestUpdate() {
    Runtime runtime;
    Check(!pal4::EffectSystemRadius::SetSquaredValue(
               0x2000u, 5.0f, runtime).has_value() &&
              runtime.squared == -1.0f &&
              runtime.calls == std::vector<std::string>{"read"},
          "disabled path does not write and has undefined upper EAX bits");
    runtime.enabled = 1u;
    runtime.calls.clear();
    Check(pal4::EffectSystemRadius::SetSquaredValue(
              0x2000u, 5.0f, runtime) == 0u &&
              runtime.squared == 25.0f &&
              runtime.calls == std::vector<std::string>{"read", "square"},
          "positive value updates squared radius");
    runtime.calls.clear();
    Check(pal4::EffectSystemRadius::SetSquaredValue(
              0x2000u, -1.0f, runtime) == 0x100u &&
              pal4::EffectSystemRadius::SetSquaredValue(
                  0x2000u, 0.0f, runtime) == 0x4000u &&
              pal4::EffectSystemRadius::SetSquaredValue(
                  0x2000u, -0.0f, runtime) == 0x4000u &&
              pal4::EffectSystemRadius::SetSquaredValue(
                  0x2000u,
                  std::numeric_limits<float>::quiet_NaN(),
                  runtime) == 0x4100u &&
              runtime.squared == 25.0f && runtime.calls.size() == 4u,
          "negative, signed zero and unordered statuses leave radius intact");
    Check(pal4::EffectSystemRadius::SetSquaredValue(
              0x2000u, std::numeric_limits<float>::infinity(),
              runtime) == 0u && std::isinf(runtime.squared),
          "positive infinity follows greater-than-zero branch");
}

}  // namespace

int main() {
    TestInitialize();
    TestUpdate();
}
