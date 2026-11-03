#include "pal4/atomic_bounding_box_builder.h"

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

struct Runtime final : pal4::IAtomicBoundingBoxRuntime {
    static constexpr std::uint32_t atomic = 0x1000u;
    static constexpr std::uint32_t box = 0x2000u;
    static constexpr std::uint32_t geometry = 0x3000u;
    static constexpr std::uint32_t morph = 0x4000u;
    static constexpr std::uint32_t object_transform = 0x5000u;
    static constexpr std::uint32_t parent_transform = 0x6000u;
    static constexpr std::uint32_t points = 0x7000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::vector<std::uint32_t> added_points;
    std::int32_t observed_count{};
    std::uint32_t requested_size{};
    std::uint32_t transformed_source{};
    pal4::AtomicMatrix transformed_matrix{};
    bool direct{true};

    Runtime() {
        words[atomic + 24u] = geometry;
        words[geometry + 20u] = 2u;
        words[geometry + 92u] = morph;
        words[morph + 20u] = 0x8000u;
        words[atomic + 4u] = 0x9000u;
        words[atomic + 60u] = 0xa000u;
        words[0xa000u + 4u] = 0xb000u;
        for (std::uint32_t index = 0u; index < 16u; ++index)
            words[object_transform + index * 4u] = index + 1u;
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        return words[address];
    }
    [[nodiscard]] std::uint32_t Allocate(
        const std::uint32_t size, const std::uint32_t flags) override {
        Check(flags == 0u, "allocates transformed vertex buffer with zero flags");
        requested_size = size;
        events.emplace_back("allocate");
        return points;
    }
    void Free(const std::uint32_t address) override {
        Check(address == points, "frees transformed point allocation");
        events.emplace_back("free");
    }
    [[nodiscard]] bool UseDirectTransform() override {
        events.emplace_back("mode");
        return direct;
    }
    [[nodiscard]] bool UseSecondaryDirectTransform() override {
        events.emplace_back("secondary-mode");
        return direct;
    }
    [[nodiscard]] std::uint32_t GetObjectTransform(
        const std::uint32_t object) override {
        if (object == 0x9000u) {
            events.emplace_back("object-transform");
            return object_transform;
        }
        Check(object == 0xb000u, "parent transform uses parent frame at +4");
        events.emplace_back("parent-transform");
        return parent_transform;
    }
    void SetupTransformMatrix(
        pal4::AtomicMatrix& destination,
        const std::uint32_t source) override {
        Check(source == parent_transform, "sets up the parent transform");
        destination.fill(0x33u);
        events.emplace_back("setup");
    }
    void MultiplyMatrices(
        pal4::AtomicMatrix& destination,
        const std::uint32_t source,
        const pal4::AtomicMatrix& parent) override {
        Check(source == object_transform && parent.front() == 0x33u &&
                  parent.back() == 0x33u,
              "multiplies object transform by prepared parent matrix");
        destination.fill(0x44u);
        events.emplace_back("multiply");
    }
    void TransformPoints(
        const std::uint32_t destination, const std::uint32_t source,
        const std::int32_t count,
        const pal4::AtomicMatrix& transform) override {
        Check(destination == points, "transforms into allocated buffer");
        observed_count = count;
        transformed_source = source;
        transformed_matrix = transform;
        events.emplace_back("transform-points");
    }
    void AddPoint(const std::uint32_t destination,
                  const std::uint32_t point) override {
        Check(destination == box, "extends supplied bounding box");
        added_points.push_back(point);
        events.emplace_back("add-point");
    }
};

using Events = std::vector<std::string>;

void TestDirectTransform() {
    Runtime runtime;
    Check(pal4::AtomicBoundingBoxBuilder::AddTransformedVertices(
              Runtime::atomic, Runtime::box, runtime) == Runtime::atomic &&
              runtime.requested_size == 24u &&
              runtime.observed_count == 2 &&
              runtime.transformed_source == 0x8000u &&
              runtime.transformed_matrix.front() == 1u &&
              runtime.transformed_matrix.back() == 16u &&
              runtime.added_points ==
                  std::vector<std::uint32_t>({Runtime::points,
                                              Runtime::points + 12u}) &&
              runtime.events == Events({"allocate", "mode",
                                        "object-transform", "transform-points",
                                        "add-point", "add-point", "free"}),
          "direct mode copies 64-byte matrix and adds transformed vertices");
}

void TestCombinedTransform() {
    Runtime runtime;
    runtime.direct = false;
    Check(pal4::AtomicBoundingBoxBuilder::AddTransformedVertices(
              Runtime::atomic, Runtime::box, runtime) == Runtime::atomic &&
              runtime.transformed_matrix.front() == 0x44u &&
              runtime.events == Events({"allocate", "mode",
                                        "object-transform", "parent-transform",
                                        "setup", "multiply", "transform-points",
                                        "add-point", "add-point", "free"}),
          "composed mode uses parent transform before vertex transform");
}

void TestEmptyAndNegativeCounts() {
    Runtime empty;
    empty.words[Runtime::geometry + 20u] = 0u;
    Check(pal4::AtomicBoundingBoxBuilder::AddTransformedVertices(
              Runtime::atomic, Runtime::box, empty) == Runtime::atomic &&
              empty.requested_size == 0u && empty.observed_count == 0 &&
              empty.added_points.empty() &&
              empty.events.back() == "free",
          "zero vertex count still transforms and frees allocation");

    Runtime negative;
    negative.words[Runtime::geometry + 20u] = 0xffffffffu;
    Check(pal4::AtomicBoundingBoxBuilder::AddTransformedVertices(
              Runtime::atomic, Runtime::box, negative) == Runtime::atomic &&
              negative.requested_size == 0xfffffff4u &&
              negative.observed_count == -1 &&
              negative.added_points.empty(),
          "negative native count wraps allocation size but skips point loop");
}

void TestContextDrivenBoundingBox() {
    Runtime direct;
    direct.direct = false;
    Check(pal4::AtomicBoundingBoxBuilder::ProcessBoundingBox(
              Runtime::atomic, 1, Runtime::box, direct) == Runtime::atomic &&
              direct.transformed_matrix.front() == 1u &&
              direct.events == Events({"allocate", "object-transform",
                  "transform-points", "add-point", "add-point", "free"}),
          "callback nonzero mode copies object matrix");

    Runtime combined;
    combined.direct = true;
    combined.words[Runtime::geometry + 20u] = 0;
    Check(pal4::AtomicBoundingBoxBuilder::ProcessBoundingBox(
              Runtime::atomic, 0, Runtime::box, combined) == Runtime::atomic &&
              combined.requested_size == 0 && combined.added_points.empty() &&
              combined.transformed_matrix.front() == 0x44u &&
              combined.events == Events({"allocate", "object-transform",
                  "parent-transform", "setup", "multiply", "transform-points",
                  "free"}),
          "callback zero mode combines matrices and frees empty buffer");
}

}  // namespace

int main() {
    TestDirectTransform();
    TestCombinedTransform();
    TestEmptyAndNegativeCounts();
    TestContextDrivenBoundingBox();
    return EXIT_SUCCESS;
}
