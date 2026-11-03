#include "pal4/d3d_face_point_projection.h"

#include <bit>
#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Trace final : pal4::ID3dTriangleTraceRuntime {
    enum class Mode { SkipClamp, ClampToCenter, ClampFails } mode;
    int classifications = 0;
    int intersections = 0;

    explicit Trace(Mode selected) : mode(selected) {}

    std::uint32_t Classify(pal4::D3dTriangleFaceState&,
                           std::uint32_t,
                           const pal4::D3dTriangleVector2&,
                           float) override {
        ++classifications;
        if (mode == Mode::SkipClamp) return 2u;
        if (mode == Mode::ClampToCenter) return 1u;
        return classifications == 1 ? 1u : 2u;
    }

    std::uint32_t Intersect(const pal4::D3dTriangleEdgeProbe&,
                            pal4::D3dTriangleFaceState&,
                            std::uint32_t,
                            pal4::D3dTriangleVector2&) override {
        ++intersections;
        return 0u;
    }
};

struct Height final : pal4::ID3dHeightRuntime {
    int calls = 0;
    float x = 0.0f;
    float z = 0.0f;
    pal4::D3dHeightResult Evaluate(float x_value,
                                   float z_value) override {
        ++calls;
        x = x_value;
        z = z_value;
        return {x + z, 0x1234u};
    }
};

void TestProjection() {
    pal4::D3dTriangleFaceState face{};
    face.words[16] = std::bit_cast<std::uint32_t>(-1.0f);
    face.words[18] = std::bit_cast<std::uint32_t>(4.0f);
    pal4::D3dTriangleVector3 output{};
    const pal4::D3dTriangleVector3 input{2.0f, 99.0f, 3.0f};

    Trace skip{Trace::Mode::SkipClamp};
    Height height;
    Check(&pal4::D3dFacePointProjection::Project(
              output, face, input, skip, height) == &output &&
              output.x == 2.0f && output.y == 5.0f &&
              output.z == 3.0f && skip.classifications == 3 &&
              skip.intersections == 0 && height.calls == 1 &&
              height.x == 2.0f && height.z == 3.0f,
          "all outside-edge classifications skip clamp but project height");

    Trace clamp{Trace::Mode::ClampToCenter};
    height = {};
    Check(&pal4::D3dFacePointProjection::Project(
              output, face, input, clamp, height) == &output &&
              output.x == -1.0f && output.y == 3.0f &&
              output.z == 4.0f && clamp.classifications == 9 &&
              clamp.intersections == 0 && height.calls == 1 &&
              height.x == -1.0f && height.z == 4.0f,
          "non-outside classification clamps to center before height query");

    Trace failure{Trace::Mode::ClampFails};
    height = {};
    output = input;
    Check(&pal4::D3dFacePointProjection::Project(
              output, face, output, failure, height) == &output &&
              output.x == 2.0f && output.y == 5.0f &&
              output.z == 3.0f && failure.intersections == 0 &&
              height.calls == 1,
          "failed clamp still projects height and output may alias input");
}

}  // namespace

int main() {
    TestProjection();
}
