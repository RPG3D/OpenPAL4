#include "pal4/atomic_bounding_box_builder.h"

#include <bit>
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

struct Runtime final : pal4::IAtomicBoxTransformRuntime {
    static constexpr std::uint32_t box = 0x1000u;
    static constexpr std::uint32_t atomic = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    pal4::AtomicBoxCorners original{};
    pal4::AtomicBoxCorners transformed{};
    std::uint32_t result{0x4444u};

    Runtime() {
        words[atomic + 4u] = 0x3000u;
        for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
            words[box + axis * 4u] =
                std::bit_cast<std::uint32_t>(static_cast<float>(axis + 1u));
            words[box + 12u + axis * 4u] =
                std::bit_cast<std::uint32_t>(static_cast<float>(axis + 4u));
        }
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        return words[address];
    }
    [[nodiscard]] std::uint32_t GetObjectTransform(
        const std::uint32_t object) override {
        Check(object == 0x3000u, "uses atomic frame at +4");
        events.emplace_back("transform");
        return 0x4000u;
    }
    void TransformCorners(
        pal4::AtomicBoxCorners& corners,
        const std::uint32_t transform) override {
        Check(transform == 0x4000u, "uses frame transform on eight points");
        original = corners;
        for (auto& coordinate : corners) coordinate += 10.0f;
        events.emplace_back("transform-corners");
    }
    [[nodiscard]] std::uint32_t CalculateBox(
        const std::uint32_t destination,
        const pal4::AtomicBoxCorners& corners) override {
        Check(destination == box + 24u,
              "writes transformed bounds at box +24");
        transformed = corners;
        events.emplace_back("calculate-box");
        return result;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::AtomicBoundingBoxBuilder::UpdateTransformedBox(
              Runtime::box, 0u, runtime) == 0u && runtime.events.empty(),
          "null atomic returns zero without RenderWare calls");
    Check(pal4::AtomicBoundingBoxBuilder::UpdateTransformedBox(
              Runtime::box, Runtime::atomic, runtime) == runtime.result &&
              runtime.events ==
                  std::vector<std::string>({"transform", "transform-corners",
                                            "calculate-box"}),
          "returns RenderWare box calculation result");
    for (std::uint32_t corner = 0u; corner < 8u; ++corner) {
        for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
            const auto expected = static_cast<float>(
                axis + (((corner & (1u << axis)) != 0u) ? 1u : 4u));
            Check(runtime.original[corner * 3u + axis] == expected &&
                      runtime.transformed[corner * 3u + axis] ==
                          expected + 10.0f,
                  "corner bit mask selects min/max for each axis in order");
        }
    }
    return EXIT_SUCCESS;
}
