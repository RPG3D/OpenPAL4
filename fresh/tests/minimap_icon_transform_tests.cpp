#include "pal4/minimap_icon_object.h"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMinimapIconTransformRuntime {
public:
    std::vector<std::string> events;
    std::vector<std::array<std::uint32_t, 2>> states;
    std::array<pal4::MinimapIm2DVertex, 4> vertices{};
    std::uint32_t primitive = 0u;
    std::uint32_t context = 0u;
    std::uint32_t texture_address = 0u;
    std::array<pal4::MinimapVector3, 3> axes{};
    std::array<float, 3> angles{};
    std::array<std::uint32_t, 3> rotation_modes{};
    std::array<pal4::MinimapVector3, 2> translations{};
    std::array<pal4::MinimapVector3, 4> input_points{};
    std::size_t point_passes = 0u;
    std::size_t rotation_count = 0u;
    std::size_t translation_count = 0u;

    float NearClip(std::uint32_t value) override {
        context = value;
        events.emplace_back("near");
        return 4.0f;
    }
    float VertexDepth() override {
        events.emplace_back("depth");
        return 0.25f;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        texture_address = address;
        events.emplace_back("raster");
        return 0x9000u;
    }
    void Translate(pal4::MinimapMatrix& matrix,
                   const pal4::MinimapVector3& offset,
                   std::uint32_t combine) override {
        Check(combine == 2u && matrix.flags == 0x20003u &&
                  matrix.right.x == 1.0f && matrix.up.y == 1.0f &&
                  matrix.at.z == 1.0f,
              "translation starts with an identity matrix");
        translations[translation_count++] = offset;
        matrix.pos = offset;
        events.emplace_back("translate");
    }
    void Rotate(pal4::MinimapMatrix& matrix,
                const pal4::MinimapVector3& axis,
                float angle, std::uint32_t combine) override {
        Check(matrix.flags == 0x20003u && matrix.pos.x == 0.0f &&
                  matrix.pos.y == 0.0f,
              "rotation starts with a separate identity matrix");
        axes[rotation_count] = axis;
        angles[rotation_count] = angle;
        rotation_modes[rotation_count++] = combine;
        events.emplace_back("rotate");
    }
    void TransformPoints(
        std::array<pal4::MinimapVector3, 4>& points,
        const pal4::MinimapMatrix& matrix) override {
        if (point_passes++ == 0u) input_points = points;
        for (auto& point : points) {
            point.x += matrix.pos.x;
            point.y += matrix.pos.y;
            point.z += matrix.pos.z;
        }
        events.emplace_back("points");
    }
    void SetRenderState(std::uint32_t state,
                        std::uint32_t value) override {
        states.push_back({state, value});
        events.emplace_back("state");
    }
    void DrawPrimitive(
        std::uint32_t kind,
        const std::array<pal4::MinimapIm2DVertex, 4>& data) override {
        primitive = kind;
        vertices = data;
        events.emplace_back("draw");
    }
};

void TestEarlyReturns() {
    pal4::MinimapIconState state;
    Runtime runtime;
    Check(!pal4::MinimapIconObject::DrawTransformed(
              state, 0u, {10u, 20u, 30u, 40u}, 255u, runtime) &&
              runtime.events.empty(),
          "null context returns before texture lookup");
    Check(!pal4::MinimapIconObject::DrawTransformed(
              state, 0x6000u, {10u, 20u, 30u, 40u}, 255u,
              runtime) && runtime.events.empty(),
          "missing icon texture returns before RenderWare calls");
}

void TestGeometryAndRenderSequence() {
    pal4::MinimapIconState state;
    state.WriteDword(0x20u, 0x7000u);
    state.WriteDword(0x38u, std::bit_cast<std::uint32_t>(10.0f));
    state.WriteDword(0x3cu, std::bit_cast<std::uint32_t>(20.0f));
    state.WriteDword(0x40u, std::bit_cast<std::uint32_t>(30.0f));
    Runtime runtime;
    Check(pal4::MinimapIconObject::DrawTransformed(
              state, 0x6000u, {10u, 20u, 30u, 40u}, 0x42u,
              runtime),
          "textured transformed rectangle draws");
    Check(runtime.context == 0x6000u &&
              runtime.translation_count == 2u &&
              runtime.translations[0].x == -25.0f &&
              runtime.translations[0].y == 0.0f &&
              runtime.translations[0].z == -40.0f &&
              runtime.translations[1].x == 25.0f &&
              runtime.translations[1].y == 0.0f &&
              runtime.translations[1].z == 40.0f,
          "opposite translations surround the rotation");
    Check(runtime.input_points[0].x == 10.0f &&
              runtime.input_points[0].y == 0.0f && runtime.input_points[0].z == 60.0f &&
              runtime.input_points[3].x == 40.0f &&
              runtime.input_points[3].y == 0.0f && runtime.input_points[3].z == 20.0f,
          "IDA icon quad lies in X/Z so actor Y rotation turns within the map");
    Check(runtime.rotation_count == 3u &&
              runtime.axes[0].x == 1.0f &&
              runtime.axes[1].y == 1.0f &&
              runtime.axes[2].z == 1.0f &&
              runtime.angles == std::array<float, 3>{10.0f, 20.0f, 30.0f} &&
              runtime.rotation_modes ==
                  std::array<std::uint32_t, 3>{0u, 2u, 2u},
          "three rotations use IDA axes, angles and combine modes");
    const auto& v = runtime.vertices;
    Check(v[0].x == 10.0f && v[0].y == 60.0f &&
              v[1].x == 40.0f && v[1].y == 60.0f &&
              v[2].x == 10.0f && v[2].y == 20.0f &&
              v[3].x == 40.0f && v[3].y == 20.0f,
          "vertices use bottom-left, bottom-right, top-left, top-right order");
    Check(v[0].u == 0.0f && v[0].v == 1.0f &&
              v[1].u == 1.0f && v[1].v == 1.0f &&
              v[2].u == 0.0f && v[2].v == 0.0f &&
              v[3].u == 1.0f && v[3].v == 0.0f,
          "vertex texture coordinates match the corner order");
    for (const auto& vertex : v) {
        Check(vertex.z == 0.25f && vertex.reciprocal_z == 0.25f &&
                  vertex.color == 0x42ffffffu,
              "each vertex receives camera depth, reciprocal near clip and ARGB");
    }
    Check(runtime.states ==
              std::vector<std::array<std::uint32_t, 2>>{
                  {12u, 1u}, {10u, 5u}, {11u, 6u}, {6u, 0u},
                  {8u, 0u}, {7u, 2u}, {9u, 2u}, {2u, 3u},
                  {1u, 0x9000u}} &&
              runtime.texture_address == 0x7000u &&
              runtime.primitive == 4u,
          "nine render states and four-vertex primitive match IDA");
    Check(runtime.events == std::vector<std::string>{
              "near", "translate", "points", "rotate", "rotate",
              "rotate", "points", "translate", "points", "depth",
              "depth", "depth", "depth", "state", "state", "state",
              "state", "state", "state", "state", "state", "raster",
              "state", "draw"},
          "matrix, vertex and rendering stages preserve binary order");
}

void TestSignedRectangleAndAlphaMask() {
    pal4::MinimapIconState state;
    state.WriteDword(0x20u, 1u);
    Runtime runtime;
    Check(pal4::MinimapIconObject::DrawTransformed(
              state, 1u,
              {std::bit_cast<std::uint32_t>(-10),
               std::bit_cast<std::uint32_t>(-20), 4u, 6u},
              0x123u, runtime) &&
              runtime.vertices[0].x == -10.0f &&
              runtime.vertices[0].y == -14.0f &&
              runtime.vertices[3].x == -6.0f &&
              runtime.vertices[3].y == -20.0f &&
              runtime.vertices[0].color == 0x23ffffffu,
          "rectangle arithmetic is signed and alpha occupies one color byte");
    pal4::MinimapIconTransformStub stub;
    Check(pal4::MinimapIconObject::DrawTransformed(
              state, 1u, {0u, 0u, 1u, 1u}, 255u, stub),
          "RenderWare-free transform stub links and completes");
}

}  // namespace

int main() {
    TestEarlyReturns();
    TestGeometryAndRenderSequence();
    TestSignedRectangleAndAlphaMask();
    return EXIT_SUCCESS;
}
