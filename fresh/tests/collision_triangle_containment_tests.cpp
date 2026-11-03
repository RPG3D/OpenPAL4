#include "pal4/collision_triangle_containment.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

using Point = pal4::CollisionPoint3;
using Triangle = std::array<Point, 3u>;

void TestDominantZ() {
    const Triangle triangle{{{0.0f, 0.0f, 0.0f},
                             {2.0f, 0.0f, 0.0f},
                             {0.0f, 2.0f, 0.0f}}};
    const Point normal{0.0f, 0.0f, -4.0f};
    Check(pal4::CollisionTriangleContainment::ContainsProjectedPoint(
              {0.25f, 0.25f, 99.0f}, triangle, normal),
          "dominant Z projects to XY without plane-distance test");
    Check(!pal4::CollisionTriangleContainment::ContainsProjectedPoint(
              {2.0f, 2.0f, 0.0f}, triangle, normal),
          "XY point beyond diagonal is outside");
    Check(pal4::CollisionTriangleContainment::ContainsProjectedPoint(
              {0.0f, 1.0f, 0.0f}, triangle, normal) &&
              !pal4::CollisionTriangleContainment::ContainsProjectedPoint(
                  {1.0f, 1.0f, 0.0f}, triangle, normal),
          "strict crossing makes left edge inside and diagonal edge outside");
    Check(pal4::CollisionTriangleContainment::ContainsProjectedPoint(
              {0.5f, 0.0f, 0.0f}, triangle, normal) &&
              !pal4::CollisionTriangleContainment::ContainsProjectedPoint(
                  {0.0f, 2.0f, 0.0f}, triangle, normal),
          "half-open vertical interval includes bottom but excludes top");

    const Triangle reversed{{triangle[2], triangle[1], triangle[0]}};
    Check(pal4::CollisionTriangleContainment::ContainsProjectedPoint(
              {0.25f, 0.25f, 0.0f}, reversed, normal),
          "edge orientation does not change interior result");
}

void TestDominantYAndTie() {
    const Triangle triangle{{{0.0f, 0.0f, 0.0f},
                             {2.0f, 0.0f, 0.0f},
                             {0.0f, 0.0f, 2.0f}}};
    Check(pal4::CollisionTriangleContainment::ContainsProjectedPoint(
              {0.25f, 40.0f, 0.25f}, triangle,
              {0.0f, 3.0f, 1.0f}) &&
              !pal4::CollisionTriangleContainment::ContainsProjectedPoint(
                  {1.5f, 0.0f, 1.5f}, triangle,
                  {0.0f, 3.0f, 1.0f}),
          "dominant Y projects to XZ");
    Check(pal4::CollisionTriangleContainment::ContainsProjectedPoint(
              {0.25f, 0.0f, 0.25f}, triangle,
              {0.0f, 1.0f, 1.0f}),
          "Y wins an absolute-normal tie with Z");
}

void TestDominantXAndTie() {
    const Triangle triangle{{{0.0f, 0.0f, 0.0f},
                             {0.0f, 2.0f, 0.0f},
                             {0.0f, 0.0f, 2.0f}}};
    Check(pal4::CollisionTriangleContainment::ContainsProjectedPoint(
              {50.0f, 0.25f, 0.25f}, triangle,
              {-3.0f, 1.0f, 2.0f}) &&
              !pal4::CollisionTriangleContainment::ContainsProjectedPoint(
                  {0.0f, 1.5f, 1.5f}, triangle,
                  {-3.0f, 1.0f, 2.0f}),
          "dominant X projects to YZ");
    Check(pal4::CollisionTriangleContainment::ContainsProjectedPoint(
              {0.0f, 0.25f, 0.25f}, triangle,
              {1.0f, 1.0f, 1.0f}),
          "X wins ties with Y and Z");

    const auto nan = std::numeric_limits<float>::quiet_NaN();
    Check(pal4::CollisionTriangleContainment::ContainsProjectedPoint(
              {0.0f, 0.25f, 0.25f}, triangle,
              {nan, 1.0f, 1.0f}),
          "unordered normal comparison falls through to X projection");
}

void TestLineIntersection() {
    const Triangle triangle{{{0.0f, 0.0f, 0.0f},
                             {2.0f, 0.0f, 0.0f},
                             {0.0f, 2.0f, 0.0f}}};
    Point intersection{9.0f, 9.0f, 9.0f};
    Check(pal4::CollisionTriangleContainment::IntersectLineWithTriangle(
              {0.5f, 0.5f, 3.0f}, {0.0f, 0.0f, -1.0f},
              triangle, &intersection) &&
              intersection == Point{0.5f, 0.5f, 0.0f},
          "interior line returns barycentric triangle point");
    Check(pal4::CollisionTriangleContainment::IntersectLineWithTriangle(
              {0.5f, 0.5f, 3.0f}, {0.0f, 0.0f, 1.0f},
              triangle, nullptr),
          "opposite line direction is accepted without ray-distance test");
    Check(pal4::CollisionTriangleContainment::IntersectLineWithTriangle(
              {1.0f, 1.0f, 3.0f}, {0.0f, 0.0f, -1.0f},
              triangle, &intersection) &&
              intersection == Point{1.0f, 1.0f, 0.0f},
          "barycentric edge with u plus v equal one is included");
    Check(!pal4::CollisionTriangleContainment::IntersectLineWithTriangle(
              {2.0f, 2.0f, 3.0f}, {0.0f, 0.0f, -1.0f},
              triangle, &intersection) &&
              intersection == Point{1.0f, 1.0f, 0.0f},
          "outside point is rejected without changing output");
    Check(!pal4::CollisionTriangleContainment::IntersectLineWithTriangle(
              {0.5f, 0.5f, 3.0f}, {1.0f, 0.0f, 0.0f},
              triangle, &intersection) &&
              intersection == Point{1.0f, 1.0f, 0.0f},
          "parallel direction is rejected without changing output");
}

}  // namespace

int main() {
    TestDominantZ();
    TestDominantYAndTie();
    TestDominantXAndTie();
    TestLineIntersection();
}
