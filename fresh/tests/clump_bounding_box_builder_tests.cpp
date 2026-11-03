#include "pal4/atomic_bounding_box_builder.h"
#include "pal4/clump_bounding_box_builder.h"

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

struct Runtime final : pal4::IAtomicBoundingBoxRuntime,
                       pal4::IClumpBoundingBoxRuntime {
    static constexpr std::uint32_t clump = 0x1000u;
    static constexpr std::uint32_t atomic = 0x2000u;
    static constexpr std::uint32_t box = 0x3000u;
    static constexpr std::uint32_t geometry = 0x4000u;
    static constexpr std::uint32_t morph = 0x5000u;
    static constexpr std::uint32_t points = 0x6000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {atomic + 24u, geometry}, {geometry + 20u, 2u},
        {geometry + 92u, morph}, {morph + 20u, 0x7000u},
        {atomic + 4u, 0x8000u}, {atomic + 60u, 0x9000u},
        {0x9000u + 4u, 0xa000u}};
    std::vector<std::string> events;
    std::vector<std::uint32_t> added_points;
    std::uint32_t transform_first = 0u;
    std::int32_t transformed_count = -1;

    Runtime() {
        for (std::uint32_t index = 0u; index < 16u; ++index)
            words[0xb000u + index * 4u] = index + 1u;
    }
    [[nodiscard]] std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void SetDirectTransformMode(const std::uint32_t value) override {
        words[0x92f560u] = value;
        events.emplace_back("mode-write");
    }
    [[nodiscard]] std::uint32_t Allocate(
        const std::uint32_t size, const std::uint32_t flags) override {
        Check(size == static_cast<std::uint32_t>(words[geometry + 20u] * 12u)
                  && flags == 0u,
              "allocates twelve bytes per geometry vertex");
        events.emplace_back("allocate");
        return points;
    }
    void Free(const std::uint32_t address) override {
        Check(address == points, "frees transformed point buffer");
        events.emplace_back("free");
    }
    [[nodiscard]] bool UseDirectTransform() override {
        Check(false, "secondary callback must not read primary mode");
        return false;
    }
    [[nodiscard]] bool UseSecondaryDirectTransform() override {
        events.emplace_back("secondary-mode");
        return words[0x92f560u] != 0u;
    }
    [[nodiscard]] std::uint32_t GetObjectTransform(
        const std::uint32_t object) override {
        if (object == 0x8000u) return 0xb000u;
        Check(object == 0xa000u, "parent frame comes from atomic +60/+4");
        return 0xc000u;
    }
    void SetupTransformMatrix(pal4::AtomicMatrix& destination,
                              const std::uint32_t source) override {
        Check(source == 0xc000u, "prepares parent matrix");
        destination.fill(0x33u);
        events.emplace_back("setup");
    }
    void MultiplyMatrices(pal4::AtomicMatrix& destination,
                          const std::uint32_t source,
                          const pal4::AtomicMatrix& parent) override {
        Check(source == 0xb000u && parent.front() == 0x33u,
              "multiplies atomic transform with prepared parent");
        destination.fill(0x44u);
        events.emplace_back("multiply");
    }
    void TransformPoints(const std::uint32_t destination,
                         const std::uint32_t source,
                         const std::int32_t count,
                         const pal4::AtomicMatrix& matrix) override {
        Check(destination == points && source == 0x7000u,
              "transforms geometry source into allocation");
        transformed_count = count;
        transform_first = matrix.front();
        events.emplace_back("transform");
    }
    void AddPoint(const std::uint32_t destination,
                  const std::uint32_t point) override {
        Check(destination == box, "adds point to supplied bounding box");
        added_points.push_back(point);
        events.emplace_back("add-point");
    }
    [[nodiscard]] std::uint32_t ForEachAtomic(
        const std::uint32_t value, const std::uint32_t destination) override {
        Check(value == clump && destination == box,
              "clump traversal receives callback box argument");
        events.emplace_back("traverse");
        Check(pal4::AtomicBoundingBoxBuilder::AddTransformedVerticesSecondary(
                  atomic, box, *this) == atomic,
              "secondary callback returns atomic");
        return 0x7777u;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::ClumpBoundingBoxBuilder::Build(
              Runtime::clump, 1u, Runtime::box, runtime) == 0x7777u &&
              runtime.words[0x92f560u] == 1u &&
              runtime.transform_first == 1u &&
              runtime.transformed_count == 2 &&
              runtime.added_points == std::vector<std::uint32_t>({
                  Runtime::points, Runtime::points + 12u}) &&
              runtime.events == std::vector<std::string>({
                  "mode-write", "traverse", "allocate", "secondary-mode",
                  "transform", "add-point", "add-point", "free"}),
          "direct secondary mode copies matrix and traverses all vertices");
    runtime.events.clear();
    runtime.added_points.clear();
    Check(pal4::ClumpBoundingBoxBuilder::Build(
              Runtime::clump, 0u, Runtime::box, runtime) == 0x7777u &&
              runtime.transform_first == 0x44u &&
              runtime.events == std::vector<std::string>({
                  "mode-write", "traverse", "allocate", "secondary-mode",
                  "setup", "multiply", "transform", "add-point",
                  "add-point", "free"}),
          "composed secondary mode prepares and multiplies parent transform");
    runtime.events.clear();
    runtime.added_points.clear();
    runtime.words[Runtime::geometry + 20u] = 0u;
    Check(pal4::ClumpBoundingBoxBuilder::Build(
              Runtime::clump, 1u, Runtime::box, runtime) == 0x7777u &&
              runtime.transformed_count == 0 && runtime.added_points.empty()
              && runtime.events.back() == "free",
          "zero vertices still allocate, transform and free without point loop");
    return EXIT_SUCCESS;
}
