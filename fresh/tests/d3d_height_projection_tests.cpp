#include "pal4/d3d_height_projection.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ID3dHeightRuntime {
    pal4::D3dTriangleVector3* point = nullptr;
    bool called = false;

    pal4::D3dHeightResult Evaluate(
        const float x, const float z) override {
        Check(point != nullptr && x == 2.0f && z == -4.0f &&
                  point->y == 99.0f,
              "height runtime receives X/Z before Y changes");
        called = true;
        return {3.5f, 0x12345678u};
    }
};

}  // namespace

int main() {
    pal4::D3dTriangleVector3 point{2.0f, 99.0f, -4.0f};
    Runtime runtime;
    runtime.point = &point;
    Check(pal4::D3dHeightProjection::Project(point, runtime) ==
              0x12345678u && runtime.called &&
              point.x == 2.0f && point.y == 3.5f && point.z == -4.0f,
          "projection stores evaluated height and forwards helper return");
}
