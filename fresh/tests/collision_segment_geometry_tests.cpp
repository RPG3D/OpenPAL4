#include "pal4/collision_segment_geometry.h"

#include <cmath>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

using Point = pal4::CollisionPoint3;

struct Runtime final : pal4::ICollisionSegmentRuntime {
    std::vector<std::pair<std::uint32_t, float>> candidates;
    std::uint32_t calls = 0u;
    pal4::CollisionSegmentQuery query{};

    void TraverseIntersections(
        const std::uint32_t world,
        const pal4::CollisionSegmentQuery& current,
        const std::uint32_t callback,
        pal4::CollisionSegmentHit& hit) override {
        Check(world == 0x5000u && callback == 0x5feae0u,
              "world traversal and tail callback address");
        Check(hit.triangle == 0u &&
                  hit.fraction == std::numeric_limits<float>::max(),
              "hit initialized before traversal");
        ++calls;
        query = current;
        for (const auto& [triangle, fraction] : candidates) {
            Check(pal4::CollisionSegmentGeometry::RecordClosestHit(
                      triangle, fraction, hit) == triangle,
                  "tail callback returns candidate triangle");
        }
    }
};

bool Near(const float actual, const float expected) {
    return std::fabs(actual - expected) < 0.00001f;
}

void TestClampedEnds() {
    const Point start{1.0f, 2.0f, 3.0f};
    const Point end{5.0f, 6.0f, 7.0f};
    Point output{};
    Check(&pal4::CollisionSegmentGeometry::ProjectPointOnSegment(
              output, {-3.0f, -2.0f, -1.0f}, start, end) == &output &&
              output == start,
          "point before start clamps to start and returns output");
    Check(&pal4::CollisionSegmentGeometry::ProjectPointOnSegment(
              output, {9.0f, 10.0f, 11.0f}, start, end) == &output &&
              output == end,
          "point beyond end clamps to end");
    Check(pal4::CollisionSegmentGeometry::ProjectPointOnSegment(
              output, start, start, end) == start &&
              pal4::CollisionSegmentGeometry::ProjectPointOnSegment(
                  output, end, start, end) == end,
          "exact endpoint projections");
}

void TestInteriorProjection() {
    const Point start{1.0f, 2.0f, 3.0f};
    const Point end{5.0f, 6.0f, 7.0f};
    Point output{};
    (void)pal4::CollisionSegmentGeometry::ProjectPointOnSegment(
        output, {3.0f, 4.0f, 5.0f}, start, end);
    Check(output == Point{3.0f, 4.0f, 5.0f},
          "on-segment point is unchanged");
    (void)pal4::CollisionSegmentGeometry::ProjectPointOnSegment(
        output, {3.0f, 4.0f, 7.0f}, start, end);
    Check(Near(output[0], 11.0f / 3.0f) &&
              Near(output[1], 14.0f / 3.0f) &&
              Near(output[2], 17.0f / 3.0f),
          "off-axis point uses dot-product fraction in three dimensions");
}

void TestCapturedM20EdgeProjections() {
    // Actual pair189 SDK sphere callbacks. Gold words follow live IDA5FE8F0
    // PC24 order; case3 is a vertical edge whose projected Y must equal input Y.
    struct Sample {std::array<std::uint32_t,3> point,start,end,expected;};
    const Sample samples[]{
        {{1146500375u,1112588567u,3288964651u},{1146999821u,1128949642u,3289065574u},
         {1146319724u,1128769970u,3288935450u},{1146345329u,1128776735u,3288940349u}},
        {{1146493093u,1107488620u,3288962553u},{1146319724u,1128769970u,3288935450u},
         {1146294252u,1088973184u,3288923611u},{1146297638u,1107516132u,3288925185u}},
        {{1146260144u,1112576016u,3288864172u},{1146266302u,1129832723u,3288882491u},
         {1146266302u,1089120656u,3288882491u},{1146266302u,1112576016u,3288882491u}}
    };
    const auto point=[](const auto& words){return Point{std::bit_cast<float>(words[0]),
        std::bit_cast<float>(words[1]),std::bit_cast<float>(words[2])};};
    for(const auto& sample:samples){Point result{};
        (void)pal4::CollisionSegmentGeometry::ProjectPointOnSegment(result,
            point(sample.point),point(sample.start),point(sample.end));
        for(unsigned i=0;i<3;++i)Check(std::bit_cast<std::uint32_t>(result[i])==sample.expected[i],
            "captured M20 edge projection preserves PC24 intermediate rounding");
    }
}

void TestDegenerateAndUnordered() {
    const Point start{1.0f, 2.0f, 3.0f};
    Point output{};
    (void)pal4::CollisionSegmentGeometry::ProjectPointOnSegment(
        output, {7.0f, 8.0f, 9.0f}, start, start);
    Check(output == start, "zero-length segment returns start without divide");
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    (void)pal4::CollisionSegmentGeometry::ProjectPointOnSegment(
        output, {nan, 0.0f, 0.0f}, start, {5.0f, 6.0f, 7.0f});
    Check(output == start, "unordered projection falls through to start");
}

void TestIntersectionValidationAndMiss() {
    const pal4::CollisionSegment segment{
        {1.0f, 2.0f, 3.0f}, {5.0f, 6.0f, 7.0f}};
    Runtime runtime;
    pal4::CollisionSegmentHit hit{0xaaaau, 0.2f, {9.0f, 8.0f, 7.0f}};
    Check(!pal4::CollisionSegmentGeometry::IntersectWorld(
              nullptr, 0x5000u, hit, 3.0f, 0x55u, runtime) &&
              !pal4::CollisionSegmentGeometry::IntersectWorld(
                  &segment, 0u, hit, 3.0f, 0x55u, runtime) &&
              runtime.calls == 0u && hit.triangle == 0xaaaau &&
              hit.fraction == 0.2f,
          "null segment or world returns before touching hit");
    Check(!pal4::CollisionSegmentGeometry::IntersectWorld(
              &segment, 0x5000u, hit, 3.0f, 0x55u, runtime) &&
              runtime.calls == 1u && hit.triangle == 0u &&
              hit.fraction == std::numeric_limits<float>::max() &&
              hit.point == Point{9.0f, 8.0f, 7.0f},
          "empty traversal leaves point untouched and returns miss");
    Check(runtime.query.type == 1u &&
              runtime.query.segment.start == segment.start &&
              runtime.query.segment.end == segment.end,
          "24-byte line copied into 28-byte type-one query");
}

void TestClosestIntersection() {
    const pal4::CollisionSegment segment{
        {1.0f, 2.0f, 3.0f}, {5.0f, 6.0f, 7.0f}};
    Runtime runtime;
    runtime.candidates = {{0x100u, 0.8f}, {0x200u, 0.25f},
                          {0x300u, 0.25f}, {0x400u, 0.6f}};
    pal4::CollisionSegmentHit hit{};
    Check(pal4::CollisionSegmentGeometry::IntersectWorld(
              &segment, 0x5000u, hit, -1.0f, 0xffffffffu, runtime) &&
              hit.triangle == 0x200u && hit.fraction == 0.25f &&
              hit.point == Point{2.0f, 3.0f, 4.0f},
          "strict closest candidate wins and point is interpolated");

    Runtime sentinel;
    sentinel.candidates = {
        {0x100u, std::numeric_limits<float>::max()},
        {0x200u, std::numeric_limits<float>::quiet_NaN()}};
    Check(!pal4::CollisionSegmentGeometry::IntersectWorld(
              &segment, 0x5000u, hit, 0.0f, 0u, sentinel) &&
              hit.triangle == 0u &&
              hit.fraction == std::numeric_limits<float>::max(),
          "maximum-float and unordered candidates do not replace sentinel");
}

}  // namespace

int main() {
    TestClampedEnds();
    TestInteriorProjection();
    TestCapturedM20EdgeProjections();
    TestDegenerateAndUnordered();
    TestIntersectionValidationAndMiss();
    TestClosestIntersection();
}
