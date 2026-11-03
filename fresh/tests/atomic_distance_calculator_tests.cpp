#include "pal4/atomic_distance_calculator.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IAtomicDistanceRuntime {
    static constexpr std::uint32_t source_atomic = 0x1000u;
    static constexpr std::uint32_t target_atomic = 0x2000u;
    static constexpr std::uint32_t source_transform = 0x4000u;
    static constexpr std::uint32_t target_transform = 0x6000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {source_atomic + 76u, 2u}, {source_atomic + 4u, 0x3000u},
        {target_atomic + 4u, 0x5000u},
        {target_transform + 48u, std::bit_cast<std::uint32_t>(1.0f)},
        {target_transform + 52u, std::bit_cast<std::uint32_t>(2.0f)},
        {target_transform + 56u, std::bit_cast<std::uint32_t>(3.0f)}};
    pal4::AtomicPosition transformed{4.0f, 6.0f, 8.0f};
    std::vector<std::string> events;

    [[nodiscard]] std::uint32_t ReadWord(const std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void ResyncInterpolatedSphere(const std::uint32_t atomic) override {
        Check(atomic == source_atomic, "resyncs source atomic only");
        events.emplace_back("resync");
    }
    [[nodiscard]] std::uint32_t GetObjectTransform(
        const std::uint32_t frame) override {
        if (frame == 0x3000u) {
            events.emplace_back("source-transform");
            return source_transform;
        }
        Check(frame == 0x5000u, "target transform uses second frame");
        events.emplace_back("target-transform");
        return target_transform;
    }
    void CalculatePosition(pal4::AtomicPosition& destination,
                           const std::uint32_t source,
                           const std::uint32_t transform) override {
        Check(source == source_atomic + 28u &&
                  transform == source_transform,
              "transforms source atomic sphere center");
        destination = transformed;
        events.emplace_back("calculate-position");
    }
};

}  // namespace

int main() {
    using Calculator = pal4::AtomicDistanceCalculator;
    Runtime runtime;
    Check(Calculator::DistanceSquared(0u, Runtime::target_atomic,
                                      runtime) == 0.0 &&
              Calculator::DistanceSquared(Runtime::source_atomic, 0u,
                                           runtime) == 0.0 &&
              runtime.events.empty(),
          "either missing atomic returns zero without native calls");
    Check(Calculator::DistanceSquared(Runtime::source_atomic,
                                      Runtime::target_atomic,
                                      runtime) == 50.0 &&
              runtime.events[1] == "resync" &&
              runtime.events[3] == "source-transform" &&
              runtime.events[4] == "calculate-position" &&
              runtime.events[6] == "target-transform",
          "resync and transform order precedes three-axis squared distance");
    runtime.events.clear();
    runtime.words[Runtime::source_atomic + 76u] = 0u;
    runtime.transformed = {1.0f, 2.0f, 3.0f};
    Check(Calculator::DistanceSquared(Runtime::source_atomic,
                                      Runtime::target_atomic,
                                      runtime) == 0.0 &&
              runtime.events[1] ==
                  "read:" + std::to_string(Runtime::source_atomic + 4u),
          "clear resync flag skips resync and identical positions yield zero");
    runtime.events.clear();
    runtime.transformed = {-2.0f, 7.0f, -1.0f};
    Check(Calculator::DistanceSquared(Runtime::source_atomic,
                                      Runtime::target_atomic,
                                      runtime) == 50.0,
          "negative and positive axis deltas are squared");
    pal4::AtomicDistanceRuntimeStub stub;
    Check(Calculator::DistanceSquared(1u, 2u, stub) == 0.0,
          "camera and RenderWare boundary stub links");
    return EXIT_SUCCESS;
}
