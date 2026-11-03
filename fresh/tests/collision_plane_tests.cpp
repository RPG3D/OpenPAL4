#include "pal4/collision_plane.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

using Point = pal4::CollisionPoint3;

struct Runtime final : pal4::ICollisionPlaneRuntime {
    Point before{};
    std::uint32_t calls = 0u;

    void Normalize(Point& normal) override {
        before = normal;
        ++calls;
        const auto length = std::sqrt(
            normal[0] * normal[0] + normal[1] * normal[1] +
            normal[2] * normal[2]);
        if (length != 0.0f) {
            for (auto& component : normal) component /= length;
        }
    }
};

void TestNullOutputAndWinding() {
    Runtime runtime;
    const Point first{0.0f, 0.0f, 1.0f};
    const Point second{1.0f, 0.0f, 1.0f};
    const Point third{0.0f, 1.0f, 1.0f};
    Check(pal4::CollisionPlane::FromPoints(
              first, second, third, nullptr, runtime) == 0.0 &&
              runtime.calls == 0u,
          "null output returns zero before RenderWare normalize");
    Point normal{};
    Check(pal4::CollisionPlane::FromPoints(
              first, second, third, &normal, runtime) == 1.0 &&
              normal == Point{0.0f, 0.0f, -1.0f} &&
              runtime.before == normal && runtime.calls == 1u,
          "point order gives negative Z normal and positive plane constant");
    Check(pal4::CollisionPlane::FromPoints(
              first, third, second, &normal, runtime) == -1.0 &&
              normal == Point{0.0f, 0.0f, 1.0f},
          "reversed winding reverses normal and plane constant");
}

void TestNormalizationAndDegeneratePoints() {
    Runtime runtime;
    Point normal{};
    Check(pal4::CollisionPlane::FromPoints(
              {2.0f, 3.0f, 4.0f},
              {2.0f, 5.0f, 4.0f},
              {5.0f, 3.0f, 4.0f},
              &normal, runtime) == -4.0 &&
              runtime.before == Point{0.0f, 0.0f, 6.0f} &&
              normal == Point{0.0f, 0.0f, 1.0f},
          "cross product is normalized before plane constant");
    Check(pal4::CollisionPlane::FromPoints(
              {1.0f, 2.0f, 3.0f},
              {2.0f, 3.0f, 4.0f},
              {3.0f, 4.0f, 5.0f},
              &normal, runtime) == 0.0 &&
              runtime.before == Point{0.0f, 0.0f, 0.0f} &&
              runtime.calls == 2u,
          "degenerate points still call normalization boundary");
}

}  // namespace

int main() {
    TestNullOutputAndWinding();
    TestNormalizationAndDegeneratePoints();
}
