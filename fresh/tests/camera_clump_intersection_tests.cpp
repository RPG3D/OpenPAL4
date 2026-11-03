#include "pal4/camera_clump_intersection.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
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

struct Runtime final : pal4::ICameraClumpIntersectionRuntime {
    static constexpr std::uint32_t record = 0x1000u;
    static constexpr std::uint32_t clump = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::vector<float>> hits;
    std::vector<std::string> events;
    std::vector<std::uint32_t> atomics{0x3000u, 0x4000u, 0x5000u};

    [[nodiscard]] std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("write:" + std::to_string(address));
    }
    void CalculatePixelRay(const std::uint32_t camera,
                           const std::uint32_t output,
                           const std::uint32_t pixel) override {
        Check(camera == 0x6000u && output == record && pixel == 0x7000u,
              "pixel ray receives camera, record and pixel pointer");
        Check(words[record + 24u] == 1u,
              "query flag is initialized before pixel ray");
        events.emplace_back("pixel-ray");
    }
    [[nodiscard]] std::uint32_t ForEachAtomic(
        const std::uint32_t value, const std::uint32_t output) override {
        Check(value == clump && output == record,
              "clump traversal receives original record");
        events.emplace_back("atomics");
        for (const auto atomic : atomics)
            Check(pal4::CameraClumpIntersection::ProcessAtomic(
                      atomic, record, *this) == atomic,
                  "atomic callback returns atomic");
        return 0x8888u;
    }
    [[nodiscard]] std::uint32_t ForEachIntersection(
        const std::uint32_t atomic, const std::uint32_t output,
        const std::uint32_t distance) override {
        Check(output == record && distance == record + 32u,
              "intersection callback updates record distance in place");
        events.emplace_back("intersections:" + std::to_string(atomic));
        for (const auto hit : hits[atomic])
            Check(pal4::CameraClumpIntersection::UpdateDistance(
                      0x1234u, 0x9abcu, hit, distance, *this) == 0x9abcu,
                  "distance callback returns native second argument");
        return 0x9999u;
    }
};

}  // namespace

int main() {
    using Query = pal4::CameraClumpIntersection;
    Runtime runtime;
    runtime.words[Runtime::record + 24u] = 0xdeadbeefu;
    Check(Query::FindNearestAtomic(Runtime::record, 0x6000u, 0u, 0x7000u,
                                   runtime) == 0u &&
              runtime.words[Runtime::record + 24u] == 0xdeadbeefu &&
              runtime.events.empty(),
          "null clump leaves query record unchanged");
    runtime.hits[0x3000u] = {12.0f, 10.0f};
    runtime.hits[0x4000u] = {20.0f};
    runtime.hits[0x5000u] = {2.0f, 5.0f};
    Check(Query::FindNearestAtomic(Runtime::record, 0x6000u,
                                   Runtime::clump, 0x7000u, runtime) ==
              0x5000u &&
              runtime.words[Runtime::record + 28u] == 0x5000u &&
              runtime.words[Runtime::record + 32u] ==
                  std::bit_cast<std::uint32_t>(2.0f) &&
              runtime.words[Runtime::record + 36u] == 3u &&
              runtime.words[0x92f558u] == 2u,
          "nearest atomic wins and global hit index tracks its traversal");
    Check(runtime.events.size() >= 7u &&
              runtime.events[0] == "write:" +
                  std::to_string(Runtime::record + 24u) &&
              runtime.events[1] == "pixel-ray" &&
              runtime.events[2] == "write:" +
                  std::to_string(Runtime::record + 28u) &&
              runtime.events[5] == "atomics",
          "query initializes ray and nearest state before traversal");
    runtime.events.clear();
    runtime.atomics = {0x3000u};
    runtime.hits[0x3000u].clear();
    Check(Query::FindNearestAtomic(Runtime::record, 0x6000u,
                                   Runtime::clump, 0x7000u, runtime) == 0u &&
              runtime.words[Runtime::record + 32u] == 0x7f7fffffu &&
              runtime.words[Runtime::record + 36u] == 1u,
          "no intersection retains max finite distance and increments index");
    runtime.words[Runtime::record + 32u] =
        std::bit_cast<std::uint32_t>(3.0f);
    Check(Query::UpdateDistance(1u, 2u, 3.0f, Runtime::record + 32u,
                                runtime) == 2u &&
              runtime.words[Runtime::record + 32u] ==
                  std::bit_cast<std::uint32_t>(3.0f),
          "equal distance does not replace the nearest value");
    (void)Query::UpdateDistance(1u, 2u,
                                std::numeric_limits<float>::quiet_NaN(),
                                Runtime::record + 32u, runtime);
    Check(runtime.words[Runtime::record + 32u] ==
              std::bit_cast<std::uint32_t>(3.0f),
          "unordered distance does not replace the nearest value");
    pal4::CameraClumpIntersectionRuntimeStub stub;
    Check(Query::FindNearestAtomic(1u, 2u, 3u, 4u, stub) == 0u,
          "RenderWare intersection boundary stub links");
    return EXIT_SUCCESS;
}
