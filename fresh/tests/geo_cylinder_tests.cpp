#include "pal4/geo_cylinder.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(float actual, float expected) {
    return std::abs(actual - expected) < 0.0002f;
}

struct Vertex {
    pal4::VisualModelVector position{};
    pal4::CylinderUv uv{};
    std::uint32_t color{};
};

struct Geometry {
    Vertex vertices[4]{};
    int writes{};
    int triangles{};
    bool finished{};
    bool destroyed{};
};

struct Runtime final : pal4::IGeoCylinderRuntime {
    std::vector<Geometry> geometries;
    std::vector<std::string> calls;
    int geometry_fail_at = -1;
    int material_count{};
    int texture_set_count{};
    int frame_count{};
    int atomic_count{};
    int captured_flags_count{};
    int added_atomics{};
    int added_children{};
    int material_flags_count{};
    std::uint32_t last_flags{};

    std::uint32_t TextureWidth(std::uint32_t texture) override {
        Check(texture == 0x5555u, "width reads supplied texture");
        calls.emplace_back("texture_width");
        return 100u;
    }
    std::uint32_t TextureHeight(std::uint32_t texture) override {
        Check(texture == 0x5555u, "height reads supplied texture");
        calls.emplace_back("texture_height");
        return 50u;
    }
    std::uint32_t CreateGeometry(std::uint32_t vertices,
                                 std::uint32_t triangles,
                                 std::uint32_t flags) override {
        Check(vertices == 4u && triangles == 2u && flags == 77u,
              "geometry uses native four-vertex two-triangle format");
        calls.emplace_back("create_geometry");
        if (static_cast<int>(geometries.size()) == geometry_fail_at)
            return 0u;
        geometries.emplace_back();
        return 0x1000u + static_cast<std::uint32_t>(geometries.size() - 1u);
    }
    Geometry& At(std::uint32_t geometry) {
        Check(geometry >= 0x1000u &&
                  geometry - 0x1000u < geometries.size(),
              "geometry handle belongs to this runtime");
        return geometries[geometry - 0x1000u];
    }
    void WriteVertex(std::uint32_t geometry, std::uint32_t index,
                     pal4::VisualModelVector position,
                     pal4::CylinderUv uv,
                     std::uint32_t color) override {
        Check(index < 4u && color == 0xffffffffu,
              "all four vertices are opaque white");
        auto& entry = At(geometry);
        Check(index == static_cast<std::uint32_t>(entry.writes),
              "vertex insertion follows native index order");
        entry.vertices[index] = {position, uv, color};
        ++entry.writes;
        calls.emplace_back("vertex");
    }
    std::uint32_t CreateMaterial() override {
        ++material_count;
        calls.emplace_back("create_material");
        return 0x2000u;
    }
    void SetMaterialTexture(std::uint32_t material,
                            std::uint32_t texture) override {
        Check(material == 0x2000u && texture == 0x5555u,
              "material receives texture when present");
        ++texture_set_count;
        calls.emplace_back("set_texture");
    }
    void SetTriangle(std::uint32_t geometry, std::uint32_t index,
                     std::uint32_t first, std::uint32_t second,
                     std::uint32_t third,
                     std::uint32_t material) override {
        Check(material == 0x2000u &&
                  ((index == 0u && first == 0u && second == 1u &&
                    third == 2u) ||
                   (index == 1u && first == 1u && second == 3u &&
                    third == 2u)),
              "triangle winding and material match IDA");
        ++At(geometry).triangles;
        calls.emplace_back("triangle");
    }
    void DestroyMaterial(std::uint32_t material) override {
        Check(material == 0x2000u, "created material is released");
        calls.emplace_back("destroy_material");
    }
    void FinishGeometry(std::uint32_t geometry) override {
        auto& entry = At(geometry);
        Check(entry.writes == 4 && entry.triangles == 2,
              "bounding sphere and unlock follow complete geometry");
        entry.finished = true;
        calls.emplace_back("finish_geometry");
    }
    std::uint32_t CreateAtomic() override {
        calls.emplace_back("create_atomic");
        return 0x3000u + static_cast<std::uint32_t>(atomic_count++);
    }
    std::uint32_t CreateFrame() override {
        calls.emplace_back("create_frame");
        return 0x4000u + static_cast<std::uint32_t>(frame_count++);
    }
    void SetAtomicFrame(std::uint32_t atomic,
                        std::uint32_t frame) override {
        Check(atomic >= 0x3000u && frame >= 0x4000u,
              "atomic attaches its new frame");
        calls.emplace_back("atomic_frame");
    }
    void SetAtomicGeometry(std::uint32_t atomic,
                           std::uint32_t geometry) override {
        Check(atomic >= 0x3000u && At(geometry).finished,
              "atomic receives completed geometry");
        calls.emplace_back("atomic_geometry");
    }
    void DestroyGeometry(std::uint32_t geometry) override {
        At(geometry).destroyed = true;
        calls.emplace_back("destroy_geometry");
    }
    void CaptureSharedAtomicFlags(std::uint32_t atomic) override {
        Check(atomic >= 0x3000u, "native shared flag captures atomic");
        ++captured_flags_count;
        calls.emplace_back("capture_flags");
    }
    std::uint32_t CreateClump() override {
        calls.emplace_back("create_clump");
        return 0x5000u;
    }
    void SetClumpFrame(std::uint32_t clump,
                       std::uint32_t frame) override {
        Check(clump == 0x5000u && frame >= 0x4000u,
              "clump receives parent frame");
        calls.emplace_back("clump_frame");
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame >= 0x4000u, "parent frame is reset");
        calls.emplace_back("identity");
    }
    void AddAtomic(std::uint32_t clump,
                   std::uint32_t atomic) override {
        Check(clump == 0x5000u && atomic >= 0x3000u,
              "all generated atomics join clump");
        ++added_atomics;
        calls.emplace_back("add_atomic");
    }
    void AddChildFrame(std::uint32_t parent,
                       std::uint32_t child) override {
        Check(parent >= 0x4000u && child >= 0x4000u &&
                  parent != child,
              "each atomic frame joins parent frame");
        ++added_children;
        calls.emplace_back("add_child");
    }
    void SetMaterialFlagsForAllAtomics(
        std::uint32_t clump, std::uint32_t flags) override {
        Check(clump == 0x5000u,
              "material flags target completed clump");
        ++material_flags_count;
        last_flags = flags;
        calls.emplace_back("material_flags");
    }
};

void TestBranches() {
    for (int bordered = 0; bordered != 2; ++bordered) {
        for (int alternate = 0; alternate != 2; ++alternate) {
            Runtime runtime;
            pal4::VisualModelState model;
            model.reserved[0] = 0xa5u;
            const pal4::CylinderCreateOptions options{
                0x5555u, 3, 8.0f, 2.0f, 3.0f,
                bordered != 0, 0.5f, 90, alternate != 0,
                true, 0x7bu};
            Check(pal4::GeoCylinder::Create(model, options, runtime) == 1u &&
                      model.active == 1u && model.property == 0x7bu &&
                      model.reserved[0] == 0xa5u &&
                      model.clump == 0x5000u &&
                      model.frame == 0x4000u +
                                         static_cast<std::uint32_t>(
                                             runtime.geometries.size()),
                  "creator initializes 52-byte model and preserves reserved byte");
            const int count = bordered != 0 ? 12 : 3;
            Check(static_cast<int>(runtime.geometries.size()) == count &&
                      runtime.material_count == count &&
                      runtime.texture_set_count == count &&
                      runtime.frame_count == count + 1 &&
                      runtime.atomic_count == count &&
                      runtime.captured_flags_count == count &&
                      runtime.added_atomics == count &&
                      runtime.added_children == count &&
                      runtime.material_flags_count == 1,
                  "geometry, material, atomic and child counts match branch");
            for (const auto& geometry : runtime.geometries)
                Check(geometry.writes == 4 && geometry.triangles == 2 &&
                          geometry.finished && geometry.destroyed,
                      "each geometry is completed and released after atomic binding");
            Check(model.material_flags == 0xfeffffffu &&
                      runtime.last_flags == model.material_flags &&
                      model.rotation.x == 0.0f &&
                      model.scale.x == 1.0f && model.scale.y == 1.0f &&
                      model.scale.z == 1.0f &&
                      model.position.z == 0.0f &&
                      runtime.calls.back() == "material_flags",
                  "final transform and clump flags match native defaults");
            const auto& first = runtime.geometries.front();
            Check(Near(first.vertices[0].position.x, 3.0f) &&
                      Near(first.vertices[0].position.y, 4.0f) &&
                      Near(first.vertices[1].position.x, 2.0f) &&
                      Near(first.vertices[1].position.y, -4.0f),
                  "first pair uses radii and centered height");
            const float expected_second_radius = bordered != 0 ? 2.5f : 3.0f;
            Check(Near(std::hypot(first.vertices[2].position.x,
                                  first.vertices[2].position.z),
                       expected_second_radius),
                  "border branch subtracts width from inner pair radius");
            Check(Near(first.vertices[0].uv.u,
                       bordered != 0 ? 0.0f : 0.005f) &&
                      Near(first.vertices[0].uv.v,
                           bordered != 0 ? 0.0f : 0.01f) &&
                      Near(first.vertices[1].uv.u,
                           alternate != 0 ? 1.005f : 0.005f) &&
                      Near(first.vertices[1].uv.v,
                           alternate != 0 ? 0.01f : 1.01f),
                  "texture dimensions contribute half-texel offsets");
            const auto& last = runtime.geometries.back();
            Check(Near(last.vertices[2].position.x,
                       bordered != 0 ? -3.0f : 0.0f) &&
                      Near(last.vertices[2].position.z,
                           bordered != 0 ? 0.0f : 3.0f),
                  "final outer angle follows bordered and plain branch step");
            if (bordered != 0) {
                Check(Near(first.vertices[2].uv.u,
                           alternate != 0 ? 0.005f : 1.0f / 6.0f + 0.005f) &&
                          Near(first.vertices[2].uv.v,
                               alternate != 0 ? 1.0f / 6.0f + 0.01f : 0.01f),
                      "bordered inner pair uses odd-angle UV fraction");
            }
        }
    }
}

void TestNoTextureAndErrors() {
    Runtime runtime;
    pal4::VisualModelState model;
    pal4::CylinderCreateOptions options{
        0u, 3, 10.0f, 1.0f, 2.0f, false, 0.0f,
        180, false, false, 9u};
    options.border_count = 2;
    try {
        (void)pal4::GeoCylinder::Create(model, options, runtime);
        Check(false, "too few borders must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "Cylinder's border number is less than 3" &&
                  runtime.calls.empty() && model.active == 0u,
              "minimum border guard throws native message before runtime calls");
    }
    options.border_count = 3;
    runtime.geometry_fail_at = 0;
    try {
        (void)pal4::GeoCylinder::Create(model, options, runtime);
        Check(false, "failed geometry must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "create RpGeometry failed in geoCylinder::create!" &&
                  runtime.calls == std::vector<std::string>{"create_geometry"} &&
                  model.active == 0u,
              "geometry failure preserves native exception and initialization gate");
    }
    runtime.geometry_fail_at = -1;
    runtime.calls.clear();
    Check(pal4::GeoCylinder::Create(model, options, runtime) == 1u &&
              runtime.texture_set_count == 0 &&
              runtime.calls.front() == "create_geometry" &&
              Near(runtime.geometries.front().vertices[0].position.y, 10.0f) &&
              Near(runtime.geometries.front().vertices[1].position.y, 0.0f) &&
              Near(runtime.geometries.front().vertices[0].uv.u, 0.0f),
          "untextured, uncentered model uses zero UV offsets and height origin");
}

}  // namespace

int main() {
    TestBranches();
    TestNoTextureAndErrors();
}
