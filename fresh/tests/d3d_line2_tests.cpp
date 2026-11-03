#include "pal4/d3d_line2.h"

#include <cmath>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestStateAndCache() {
    pal4::D3dLine2State line{};
    line.normal = {17.0f, 18.0f};
    line.normal_cached = 7;
    Check(&pal4::D3dLine2::SetEndpoints(line, {1.0f, 2.0f},
                                       {4.0f, 6.0f}) == &line &&
              line.start.x == 1.0f && line.start.z == 2.0f &&
              line.end.x == 4.0f && line.end.z == 6.0f &&
              line.normal_cached == 0 && line.normal.x == 17.0f,
          "set endpoints clears only the cached flag");
    Check(&pal4::D3dLine2::Normal(line) == &line.normal &&
              std::fabs(line.normal.x - 0.8f) < 0.000001f &&
              std::fabs(line.normal.z + 0.6f) < 0.000001f &&
              line.normal_cached == 1,
          "normal rotates normalized end-minus-start clockwise");
    line.normal = {9.0f, 8.0f};
    Check(&pal4::D3dLine2::Normal(line) == &line.normal &&
              line.normal.x == 9.0f && line.normal.z == 8.0f,
          "cached normal is returned without recomputation");
    pal4::D3dLine2::ClearNormalCache(line);
    Check(line.normal_cached == 0 && line.normal.x == 9.0f,
          "clear cache preserves normal words");
    Check(&pal4::D3dLine2::SetStart(line, {0.0f, 0.0f}) == &line.start &&
              line.start.x == 0.0f && line.normal_cached == 0,
          "set start returns its field and clears cache");
    line.normal_cached = 1;
    Check(&pal4::D3dLine2::SetEnd(line, {0.0f, 2.0f}) == &line.end &&
              line.end.z == 2.0f && line.normal_cached == 0,
          "set end returns its field and clears cache");
    Check(&pal4::D3dLine2::Reinitialize(line, {2.0f, 3.0f},
                                       {4.0f, 3.0f}) == &line.end &&
              line.start.x == 2.0f && line.end.x == 4.0f &&
              line.normal_cached == 0,
          "second two-endpoint entry updates both fields");
    Check(&pal4::D3dLine2::End(line) == &line.end,
          "end accessor returns the second vector address");
    pal4::D3dTriangleVector2 direction{};
    Check(&pal4::D3dLine2::ComputeUnitDirection(line, direction) ==
              &direction && direction.x == 1.0f &&
              direction.z == 0.0f && line.normal_cached == 0,
          "unit direction helper normalizes end-minus-start without caching");
    Check(&pal4::D3dLine2::ComputeNormal(line) == &line.normal &&
              line.normal.x == 0.0f && line.normal.z == -1.0f &&
              line.normal_cached == 1,
          "normal helper rotates and sets the cache flag");
    line.normal = {7.0f, 8.0f};
    pal4::D3dLine2::ComputeNormal(line);
    Check(line.normal.x == 0.0f && line.normal.z == -1.0f,
          "direct normal helper recalculates even when cached");
}

void TestSignedDistanceAndClassification() {
    pal4::D3dLine2State line{};
    pal4::D3dLine2::SetEndpoints(line, {0.0f, 0.0f}, {4.0f, 0.0f});
    Check(pal4::D3dLine2::SignedDistance(line, {2.0f, -3.0f}) == 3.0 &&
              pal4::D3dLine2::SignedDistance(line, {2.0f, 3.0f}) == -3.0,
          "signed distance uses clockwise unit normal");
    Check(pal4::D3dLine2::Classify(line, {2.0f, -2.0f}, 1.0f) == 2 &&
              pal4::D3dLine2::Classify(line, {2.0f, 2.0f}, 1.0f) == 1 &&
              pal4::D3dLine2::Classify(line, {2.0f, 0.5f}, 1.0f) == 0,
          "classify both outside sides and tolerance band");
    Check(pal4::D3dLine2::Classify(line, {2.0f, -1.0f}, 1.0f) == 0 &&
              pal4::D3dLine2::Classify(line, {2.0f, 1.0f}, 1.0f) == 0,
          "tolerance boundaries remain inside band");
    const float nan = std::numeric_limits<float>::quiet_NaN();
    Check(pal4::D3dLine2::Classify(line, {nan, 0.0f}, 1.0f) == 0,
          "unordered distance follows middle classification");
    pal4::D3dLine2::SetEndpoints(line, {1.0f, 1.0f}, {1.0f, 1.0f});
    Check(pal4::D3dLine2::Classify(line, {2.0f, 1.0f}, 0.0f) == 0,
          "degenerate line has zero normal and distance");
    // Actual M11 face908 edge1 and its cached normal from pair132. The
    // midpoint lies exactly on the original PC24 boundary. Its classification
    // must not send path projection through the 0.9 inward-clamp branch.
    line.start = {std::bit_cast<float>(0x44d6d1b1u), std::bit_cast<float>(0x45834b2eu)};
    line.end = {std::bit_cast<float>(0x44cd2915u), std::bit_cast<float>(0x457b1294u)};
    line.normal = {std::bit_cast<float>(0xbf6c13ebu), std::bit_cast<float>(0x3ec605ddu)};
    line.normal_cached = 1;
    const pal4::D3dTriangleVector2 midpoint{
        std::bit_cast<float>(0x44d1fd63u), std::bit_cast<float>(0x45806a3cu)};
    Check(pal4::D3dLine2::SignedDistance(line, midpoint) == 0.0 &&
              pal4::D3dLine2::Classify(line, midpoint, 0.0f) == 0,
          "captured M11 edge midpoint retains zero-threshold boundary without epsilon");
}

void TestIntersection() {
    pal4::D3dLine2State first{};
    pal4::D3dLine2State second{};
    pal4::D3dLine2::SetEndpoints(first, {0.0f, 0.0f}, {4.0f, 0.0f});
    pal4::D3dTriangleVector2 point{99.0f, 99.0f};
    pal4::D3dLine2::SetEndpoints(second, {2.0f, -2.0f},
                                {2.0f, 2.0f});
    Check(pal4::D3dLine2::Intersect(first, second, &point) == 2 &&
              point.x == 2.0f && point.z == 0.0f,
          "both segments intersect");
    Check(pal4::D3dLine2::Intersect(first, second, nullptr) == 2,
          "null output still returns the classification");

    pal4::D3dLine2::SetEndpoints(second, {6.0f, -2.0f},
                                {6.0f, 2.0f});
    Check(pal4::D3dLine2::Intersect(first, second, &point) == 3 &&
              point.x == 6.0f && point.z == 0.0f,
          "only second segment contains the infinite-line intersection");
    pal4::D3dLine2::SetEndpoints(second, {2.0f, 2.0f},
                                {2.0f, 4.0f});
    Check(pal4::D3dLine2::Intersect(first, second, &point) == 4 &&
              point.x == 2.0f && point.z == 0.0f,
          "only first segment contains the infinite-line intersection");
    pal4::D3dLine2::SetEndpoints(second, {6.0f, 2.0f},
                                {6.0f, 4.0f});
    Check(pal4::D3dLine2::Intersect(first, second, &point) == 1 &&
              point.x == 6.0f && point.z == 0.0f,
          "neither segment contains the infinite-line intersection");

    point = {99.0f, 99.0f};
    pal4::D3dLine2::SetEndpoints(second, {0.0f, 1.0f},
                                {4.0f, 1.0f});
    Check(pal4::D3dLine2::Intersect(first, second, &point) == 5 &&
              point.x == 99.0f && point.z == 99.0f,
          "parallel separated lines leave output unchanged");
    pal4::D3dLine2::SetEndpoints(second, {5.0f, 0.0f},
                                {6.0f, 0.0f});
    Check(pal4::D3dLine2::Intersect(first, second, &point) == 0 &&
              point.x == 99.0f && point.z == 99.0f,
          "collinear disjoint segments still use collinear line status");
    pal4::D3dLine2::SetEndpoints(second, {0.0f, -1.0f},
                                {0.0f, 1.0f});
    Check(pal4::D3dLine2::Intersect(first, second, &point) == 2 &&
              point.x == 0.0f && point.z == 0.0f,
          "intersection at first segment endpoint is included");
}

}  // namespace

int main() {
    TestStateAndCache();
    TestSignedDistanceAndClassification();
    TestIntersection();
}
