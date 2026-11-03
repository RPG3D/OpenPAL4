#include "pal4/strip_atomic_object.h"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IStripAtomicObjectRuntime {
    std::uint32_t geometry = 0x200u;
    std::uint32_t material = 0x300u;
    std::uint32_t destroy_result = 0x9876u;
    std::vector<std::string> calls;

    std::uint32_t GeometryOf(std::uint32_t atomic) override {
        Check(atomic == 0x100u, "geometry comes from atomic");
        calls.emplace_back("geometry");
        return geometry;
    }
    std::uint32_t FirstMaterialOf(std::uint32_t value) override {
        Check(value == geometry, "material comes from geometry");
        calls.emplace_back("material");
        return material;
    }
    std::uint32_t FrameOf(std::uint32_t atomic) override {
        Check(atomic == 0x100u, "frame comes from atomic");
        calls.emplace_back("frame");
        return 0x400u;
    }
    void DestroyMaterial(std::uint32_t value) override {
        Check(value == material, "destroys first material");
        calls.emplace_back("destroy_material");
    }
    void DestroyFrame(std::uint32_t frame) override {
        Check(frame == 0x400u, "destroys atomic frame");
        calls.emplace_back("destroy_frame");
    }
    void SetAtomicFrame(std::uint32_t atomic,
                        std::uint32_t frame) override {
        Check(atomic == 0x100u && frame == 0u,
              "clears atomic frame before destroy");
        calls.emplace_back("clear_frame");
    }
    std::uint32_t DestroyAtomic(std::uint32_t atomic) override {
        Check(atomic == 0x100u, "destroys original atomic");
        calls.emplace_back("destroy_atomic");
        return destroy_result;
    }
};

struct CreateRuntime final : pal4::IStripAtomicCreateRuntime {
    struct Vertex {
        pal4::StripPoint point;
        pal4::StripUv uv;
    };
    struct Triangle {
        std::uint32_t first;
        std::uint32_t second;
        std::uint32_t third;
        std::uint32_t material;
    };
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    std::vector<std::string> calls;
    std::uint32_t geometry_result = 0x200u;
    std::uint32_t material_result = 0x300u;
    std::uint32_t cached_callback{};
    std::uint32_t atomic_callback = 0x9876u;
    bool zero_installed_callback{};
    bool installed_blended{};
    pal4::StripBoundingSphere stored_sphere{};

    std::uint32_t CreateMaterial() override {
        calls.emplace_back("material_create");
        return material_result;
    }
    std::uint32_t CreateGeometry(std::uint32_t vertices_count,
                                 std::uint32_t triangle_count,
                                 std::uint32_t flags) override {
        Check(vertices_count == 6u && triangle_count == 4u && flags == 52u,
              "geometry shape and native flags");
        calls.emplace_back("geometry_create");
        return geometry_result;
    }
    void WriteVertex(std::uint32_t geometry, std::uint32_t index,
                     pal4::StripPoint point, pal4::StripUv uv) override {
        Check(geometry == geometry_result && index == vertices.size(),
              "vertices written in array order");
        calls.emplace_back("vertex");
        vertices.push_back({point, uv});
    }
    void SetTriangle(std::uint32_t geometry, std::uint32_t index,
                     std::uint32_t first, std::uint32_t second,
                     std::uint32_t third, std::uint32_t material) override {
        Check(geometry == geometry_result && index == triangles.size(),
              "triangles written in array order");
        calls.emplace_back("triangle");
        triangles.push_back({first, second, third, material});
    }
    std::uint32_t CreateAtomic() override {
        calls.emplace_back("atomic_create");
        return 0x100u;
    }
    std::uint32_t CreateFrame() override {
        calls.emplace_back("frame_create");
        return 0x400u;
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x400u, "identity frame handle");
        calls.emplace_back("frame_identity");
    }
    void SetAtomicFrame(std::uint32_t atomic, std::uint32_t frame) override {
        Check(atomic == 0x100u && frame == 0x400u,
              "atomic frame attachment");
        calls.emplace_back("atomic_frame");
    }
    void SetAtomicGeometry(std::uint32_t atomic,
                           std::uint32_t geometry) override {
        Check(atomic == 0x100u && geometry == geometry_result,
              "atomic geometry attachment");
        calls.emplace_back("atomic_geometry");
    }
    void CalculateVertexNormals(std::uint32_t geometry) override {
        Check(geometry == geometry_result, "normal geometry handle");
        calls.emplace_back("normals");
    }
    pal4::StripBoundingSphere CalculateBoundingSphere(
        std::uint32_t geometry) override {
        Check(geometry == geometry_result, "sphere geometry handle");
        calls.emplace_back("sphere_calculate");
        return {1.0f, 2.0f, 3.0f, 4.0f};
    }
    void SetBoundingSphere(std::uint32_t geometry,
                           pal4::StripBoundingSphere sphere) override {
        Check(geometry == geometry_result, "sphere target geometry");
        calls.emplace_back("sphere_store");
        stored_sphere = sphere;
    }
    void UnlockGeometry(std::uint32_t geometry) override {
        Check(geometry == geometry_result, "unlock target geometry");
        calls.emplace_back("unlock");
    }
    void DestroyGeometry(std::uint32_t geometry) override {
        Check(geometry == geometry_result, "destroy target geometry");
        calls.emplace_back("geometry_destroy");
    }
    std::uint32_t CachedOriginalCallback() override {
        calls.emplace_back("cached_callback");
        return cached_callback;
    }
    std::uint32_t AtomicCallback(std::uint32_t atomic) override {
        Check(atomic == 0x100u, "callback atomic handle");
        calls.emplace_back("atomic_callback");
        return atomic_callback;
    }
    void CacheOriginalCallback(std::uint32_t callback) override {
        calls.emplace_back("cache_callback");
        cached_callback = callback;
    }
    void InstallCallback(std::uint32_t atomic, bool blended) override {
        Check(atomic == 0x100u, "install callback atomic");
        calls.emplace_back("install_callback");
        installed_blended = blended;
        atomic_callback = zero_installed_callback ? 0u : 0x4321u;
    }
    void InstallDefaultCallback(std::uint32_t atomic) override {
        Check(atomic == 0x100u, "fallback callback atomic");
        calls.emplace_back("default_callback");
        atomic_callback = 0x8888u;
    }
};

void CylinderPath() {
    std::vector<pal4::StripPoint> points{{99.0f, 98.0f, 97.0f}};
    Check(pal4::StripAtomicObject::AppendCylinderPath(
              points, 4, 8.0f, 6.0f, 2.0f, 2) == 0,
          "positive loop count runs to zero");
    Check(points.size() == 9u && points.front().x == 99.0f,
          "generated points append after existing centerline");
    constexpr float epsilon = 0.00001f;
    for (int i = 0; i < 8; ++i) {
        const auto& point = points[static_cast<std::size_t>(i) + 1u];
        const float expected_y = static_cast<float>(i + 1);
        const float expected_radius = 2.0f + expected_y * 0.5f;
        const float angle = static_cast<float>(i % 4) *
                            static_cast<float>(6.2831855 / 4.0);
        Check(std::abs(point.y - expected_y) < epsilon &&
                  std::abs(point.x - std::cos(angle) * expected_radius) <
                      epsilon &&
                  std::abs(point.z - std::sin(angle) * expected_radius) <
                      epsilon,
              "cylinder path angle, vertical increment and radius taper");
    }
    const auto old_size = points.size();
    Check(pal4::StripAtomicObject::AppendCylinderPath(
              points, 4, 8.0f, 6.0f, 2.0f, 0) == 0 &&
              points.size() == old_size,
          "zero loops leave path unchanged");
    Check(pal4::StripAtomicObject::AppendCylinderPath(
              points, 4, 8.0f, 6.0f, 2.0f, -2) == -2 &&
              points.size() == old_size,
          "negative loops preserve native initial return");
    Check(pal4::StripAtomicObject::AppendCylinderPath(
              points, 0, 8.0f, 6.0f, 2.0f, 2) == 0 &&
              points.size() == old_size,
          "zero borders produce no points");
}

void Create() {
    const std::vector<pal4::StripPoint> points{
        {1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f}};
    for (bool alternate : {false, true}) {
        CreateRuntime runtime;
        pal4::StripAtomicState state;
        pal4::StripAtomicObject::Create(
            state, 0x300u, points, 4.0f, alternate, alternate, runtime);
        Check(state.atomic == 0x100u, "stores created atomic");
        Check(runtime.vertices.size() == 6u &&
                  runtime.triangles.size() == 4u,
              "writes complete strip geometry");
        for (std::size_t i = 0; i < points.size(); ++i) {
            const auto& top = runtime.vertices[2u * i];
            const auto& bottom = runtime.vertices[2u * i + 1u];
            const float fraction = static_cast<float>(i) / 2.0f;
            Check(top.point.x == points[i].x &&
                      top.point.y == points[i].y + 2.0f &&
                      top.point.z == points[i].z &&
                      bottom.point.x == points[i].x &&
                      bottom.point.y == points[i].y - 2.0f &&
                      bottom.point.z == points[i].z,
                  "top and bottom coordinates preserve center line");
            Check(top.uv.u == (alternate ? 0.0f : fraction) &&
                      top.uv.v == (alternate ? fraction : 0.0f) &&
                      bottom.uv.u == (alternate ? 1.0f : fraction) &&
                      bottom.uv.v == (alternate ? fraction : 1.0f),
                  "native UV orientation and index fraction");
        }
        Check(runtime.triangles[0].first == 2u &&
                  runtime.triangles[0].second == 3u &&
                  runtime.triangles[0].third == 1u &&
                  runtime.triangles[1].first == 0u &&
                  runtime.triangles[1].second == 2u &&
                  runtime.triangles[1].third == 1u &&
                  runtime.triangles[2].first == 4u &&
                  runtime.triangles[3].first == 2u,
              "native triangle winding for both segments");
        for (const auto& triangle : runtime.triangles)
            Check(triangle.material == 0x300u,
                  "same material assigned to every triangle");
        Check(runtime.stored_sphere.x == 1.0f &&
                  runtime.stored_sphere.radius == 4.0f,
              "stores calculated bounding sphere");
        Check(runtime.cached_callback == 0x9876u &&
                  runtime.installed_blended == alternate,
              "captures original callback and selects variant");
        Check(runtime.calls == std::vector<std::string>{
            "geometry_create", "vertex", "vertex", "vertex", "vertex",
            "vertex", "vertex", "triangle", "triangle", "triangle",
            "triangle", "atomic_create", "frame_create", "frame_identity",
            "atomic_frame", "atomic_geometry", "normals",
            "sphere_calculate", "sphere_store", "unlock",
            "geometry_destroy", "cached_callback", "atomic_callback",
            "cache_callback", "install_callback", "atomic_callback"},
            "complete creation call order");
    }
}

void CreateBranches() {
    const std::vector<pal4::StripPoint> points{
        {1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f}};
    for (const auto& invalid :
         {std::vector<pal4::StripPoint>{},
          std::vector<pal4::StripPoint>{{1.0f, 2.0f, 3.0f}}}) {
        CreateRuntime runtime;
        pal4::StripAtomicState state;
        pal4::StripAtomicObject::Create(
            state, 0u, invalid, 4.0f, false, false, runtime);
        Check(runtime.calls.empty() && state.atomic == 0u,
              "short centerline exits before material creation");
    }
    CreateRuntime runtime;
    pal4::StripAtomicState state;
    pal4::StripAtomicObject::Create(
        state, 0u, points, 0.0f, false, false, runtime);
    Check(runtime.calls.empty(), "zero height exits early");
    pal4::StripAtomicObject::Create(
        state, 0u, points, std::nanf(""), false, false, runtime);
    Check(runtime.calls.empty(), "unordered height exits early");
    runtime.geometry_result = 0u;
    pal4::StripAtomicObject::Create(
        state, 0u, points, 4.0f, false, false, runtime);
    Check(runtime.calls == std::vector<std::string>{
        "material_create", "geometry_create"},
        "geometry failure stops after material creation");
    Check(state.atomic == 0u, "geometry failure keeps empty atomic");

    CreateRuntime fallback;
    fallback.material_result = 0u;
    fallback.cached_callback = 0xaaaau;
    fallback.zero_installed_callback = true;
    pal4::StripAtomicObject::Create(
        state, 0u, points, 4.0f, false, true, fallback);
    Check(state.atomic == 0x100u &&
              fallback.cached_callback == 0xaaaau &&
              fallback.atomic_callback == 0x8888u,
          "cached callback stays intact and null installed callback falls back");
    Check(std::find(fallback.calls.begin(), fallback.calls.end(),
                    "normals") == fallback.calls.end(),
          "zero material skips normals");
    Check(fallback.calls.back() == "default_callback",
          "default callback installed last");
}

void RunCase(std::uint32_t geometry, std::uint32_t material,
             const std::vector<std::string>& expected) {
    Runtime runtime;
    runtime.geometry = geometry;
    runtime.material = material;
    pal4::StripAtomicState state{0x100u};
    Check(pal4::StripAtomicObject::Destroy(state, runtime) == 0x9876u,
          "forwards atomic destruction result");
    Check(state.atomic == 0u, "clears stored atomic");
    Check(runtime.calls == expected, "native destruction call order");
    runtime.calls.clear();
    Check(pal4::StripAtomicObject::Destroy(state, runtime) == 0u,
          "second destruction is inactive");
    Check(runtime.calls.empty(), "inactive destruction makes no calls");
}

}  // namespace

int main() {
    CylinderPath();
    {
        Runtime frame_runtime;
        const pal4::StripAtomicState empty;
        Check(pal4::StripAtomicObject::GetFrame(
                  empty, frame_runtime) == 0u,
              "empty atomic has no frame");
        Check(frame_runtime.calls.empty(),
              "empty frame lookup makes no runtime call");
        const pal4::StripAtomicState populated{0x100u};
        Check(pal4::StripAtomicObject::GetFrame(
                  populated, frame_runtime) == 0x400u,
              "returns attached frame");
        Check(frame_runtime.calls == std::vector<std::string>{"frame"},
              "frame lookup reads atomic once");
    }
    Create();
    CreateBranches();
    RunCase(0x200u, 0x300u,
            {"geometry", "material", "destroy_material", "frame",
             "destroy_frame", "clear_frame", "destroy_atomic"});
    RunCase(0x200u, 0u,
            {"geometry", "material", "frame", "destroy_frame",
             "clear_frame", "destroy_atomic"});
    RunCase(0u, 0x300u,
            {"geometry", "frame", "destroy_frame", "clear_frame",
             "destroy_atomic"});
    pal4::StripAtomicState empty;
    Runtime runtime;
    Check(pal4::StripAtomicObject::Destroy(empty, runtime) == 0u,
          "empty atomic returns zero");
    Check(runtime.calls.empty(), "empty atomic makes no calls");
    std::cout << "strip atomic object tests passed\n";
}
