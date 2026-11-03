#include "pal4/atomic_bounding_box_builder.h"
#include "pal4/atomic_position_updater.h"

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

struct Runtime final : pal4::IAtomicBoxTransformRuntime,
                       pal4::IAtomicPositionRuntime {
    static constexpr std::uint32_t record = 0x1000u;
    static constexpr std::uint32_t atomic = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    Runtime() {
        words[atomic + 4u] = 0x3000u;
        words[record + 60u] = 0x12345678u;
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check(address == record + 76u && value == 0x12345678u,
              "copies camera field after both operations");
        words[address] = value;
        events.emplace_back("write-camera");
    }
    [[nodiscard]] std::uint32_t GetObjectTransform(
        const std::uint32_t object) override {
        Check(object == 0x3000u, "both entries use the same atomic frame");
        events.emplace_back("get-transform");
        return 0x4000u;
    }
    void TransformCorners(pal4::AtomicBoxCorners&,
                          const std::uint32_t transform) override {
        Check(transform == 0x4000u, "transforms corners with atomic matrix");
        events.emplace_back("transform-corners");
    }
    [[nodiscard]] std::uint32_t CalculateBox(
        const std::uint32_t destination,
        const pal4::AtomicBoxCorners&) override {
        Check(destination == record + 24u, "calculates transformed box first");
        events.emplace_back("calculate-box");
        return 0x9999u;
    }
    void CalculatePosition(const std::uint32_t destination,
                           const std::uint32_t source,
                           const std::uint32_t transform) override {
        Check(destination == record + 64u && source == record + 48u &&
                  transform == 0x4000u,
              "updates camera position after the transformed box");
        events.emplace_back("calculate-position");
    }
    void ResyncInterpolatedSphere(std::uint32_t) override {}
    [[nodiscard]] bool UseDirectTransform() override { return true; }
    void SetupTransformMatrix(pal4::AtomicMatrix&,
                              std::uint32_t) override {}
    void MultiplyMatrices(pal4::AtomicMatrix&, std::uint32_t,
                          const pal4::AtomicMatrix&) override {}
    void TransformSphere(pal4::AtomicSphere&,
                         const pal4::AtomicMatrix&) override {}
    [[nodiscard]] bool IsInitialSphere() override { return true; }
    void ClearInitialSphere() override {}
    [[nodiscard]] float Length(const pal4::AtomicVector3&) override {
        return 0.0f;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::AtomicBoundingBoxBuilder::UpdateBoxAndCamera(
              Runtime::record, 0u, runtime, runtime) == 0u &&
              runtime.events.empty(),
          "null atomic leaves both box and camera untouched");
    Check(pal4::AtomicBoundingBoxBuilder::UpdateBoxAndCamera(
              Runtime::record, Runtime::atomic, runtime, runtime) ==
              0x12345678u &&
              runtime.events == std::vector<std::string>({
                  "get-transform", "transform-corners", "calculate-box",
                  "get-transform", "calculate-position", "write-camera"}),
          "box update precedes camera update and latter result is returned");
    return EXIT_SUCCESS;
}
