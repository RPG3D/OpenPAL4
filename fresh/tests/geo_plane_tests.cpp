#include "pal4/geo_plane.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IGeoPlaneRuntime {
    std::uint32_t clump{};
    std::uint32_t flags{};
    int material_calls{};
    int transform_calls{};
    std::vector<std::string> transform_events;
    pal4::GeoPlaneVector last_scale{};
    pal4::GeoPlaneVector last_position{};
    int SetMaterialFlagsForAllAtomics(
        std::uint32_t input_clump, std::uint32_t input_flags) override {
        clump = input_clump;
        flags = input_flags;
        ++material_calls;
        return -17;
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x8100u, "setter updates plane frame");
        transform_events.emplace_back("identity");
    }
    void RotateFrame(std::uint32_t frame, pal4::GeoPlaneVector axis,
                     float angle, std::uint32_t combine) override {
        Check(frame == 0x8100u, "rotation uses plane frame");
        const auto name = axis.x == 1.f ? "x" : axis.y == 1.f ? "y" : "z";
        transform_events.push_back(std::string("rotate:") + name + ":" +
            std::to_string(static_cast<int>(angle)) + ":" +
            std::to_string(combine));
    }
    void ScaleFrame(std::uint32_t frame, pal4::GeoPlaneVector scale,
                    std::uint32_t combine) override {
        Check(frame == 0x8100u, "scale uses plane frame");
        last_scale = scale;
        transform_events.push_back("scale:" + std::to_string(combine));
    }
    void TranslateFrame(std::uint32_t frame, pal4::GeoPlaneVector position,
                        std::uint32_t combine) override {
        Check(frame == 0x8100u, "translation uses plane frame");
        last_position = position;
        transform_events.push_back("translate:" + std::to_string(combine));
    }
    int UpdateFrameObjects(std::uint32_t frame) override {
        Check(frame == 0x8100u, "objects update uses plane frame");
        transform_events.emplace_back("update");
        ++transform_calls;
        return 29;
    }
};

struct GeometryRuntime final : pal4::IGeoPlaneGeometryRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::string> calls;
    int unlock_result = 0x193;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        writes.emplace_back(address, value);
        words[address] = value;
    }
    void LockGeometry(std::uint32_t geometry,
                      std::uint32_t flags) override {
        Check(geometry == 0x3000u && flags == 16u,
              "geometry is locked for vertex writes");
        calls.emplace_back("lock");
    }
    int UnlockGeometry(std::uint32_t geometry) override {
        Check(geometry == 0x3000u, "same geometry is unlocked");
        calls.emplace_back("unlock");
        return unlock_result;
    }
    void ClearCalls() {
        writes.clear();
        calls.clear();
    }
};

struct CreateRuntime final : pal4::IGeoPlaneCreateRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t shared_callback{};
    std::uint32_t next_frame = 0x8000u;
    bool geometry_failure{};
    bool saw_final_state{};

    CreateRuntime() {
        words[0x202cu] = 0x3000u;
        words[0x205cu] = 0x4000u;
        words[0x4014u] = 0x5000u;
        words[0x2034u] = 0x6000u;
        words[0x2030u] = 0x6100u;
        words[0x7048u] = 0xf00du;
    }
    std::uint32_t ClumpWorld(std::uint32_t) override {
        calls.emplace_back("get_world");
        return 0u;
    }
    void RemoveClumpFromWorld(std::uint32_t,
                              std::uint32_t) override {
        calls.emplace_back("remove_world");
    }
    void ClearClumpFrame(std::uint32_t) override {
        calls.emplace_back("clear_clump_frame");
    }
    void DestroyClump(std::uint32_t) override {
        calls.emplace_back("destroy_clump");
    }
    void DestroyFrame(std::uint32_t) override {
        calls.emplace_back("destroy_frame");
    }
    std::uint32_t CreateMaterial() override {
        calls.emplace_back("create_material");
        return 0x1000u;
    }
    void SetMaterialTexture(std::uint32_t material,
                            std::uint32_t texture) override {
        Check(material == 0x1000u && texture == 0xabcdu,
              "texture is assigned to created material");
        calls.emplace_back("set_texture");
    }
    std::uint32_t CreateGeometry(std::uint32_t vertices,
                                 std::uint32_t triangles,
                                 std::uint32_t flags) override {
        Check(vertices == 4u && triangles == 2u && flags == 77u,
              "geometry has four vertices, two triangles and flag 77");
        calls.emplace_back("create_geometry");
        return geometry_failure ? 0u : 0x2000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void SetTriangleIndices(std::uint32_t geometry,
                            std::uint32_t triangle,
                            std::array<std::uint32_t, 3u> indices) override {
        Check(geometry == 0x2000u &&
                  ((triangle == 0x3000u && indices ==
                    std::array<std::uint32_t, 3u>{0u, 1u, 2u}) ||
                   (triangle == 0x3008u && indices ==
                    std::array<std::uint32_t, 3u>{1u, 3u, 2u})),
              "native triangle winding is retained");
        calls.emplace_back("triangle:" + std::to_string(triangle));
    }
    void SetTriangleMaterial(std::uint32_t geometry,
                             std::uint32_t triangle,
                             std::uint32_t material) override {
        Check(geometry == 0x2000u && material == 0x1000u,
              "both triangles use the created material");
        calls.emplace_back("triangle_material:" +
                           std::to_string(triangle));
    }
    void DestroyMaterial(std::uint32_t material) override {
        Check(material == 0x1000u, "material reference is released");
        calls.emplace_back("destroy_material");
    }
    std::array<std::uint32_t, 4u> CalculateBoundingSphere(
        std::uint32_t morph_target) override {
        Check(morph_target == 0x4000u,
              "bounding sphere uses geometry morph target");
        calls.emplace_back("sphere");
        return {11u, 22u, 33u, 44u};
    }
    void UnlockGeometry(std::uint32_t geometry) override {
        Check(geometry == 0x2000u, "created geometry is unlocked");
        calls.emplace_back("unlock_geometry");
    }
    std::uint32_t CreateAtomic() override {
        calls.emplace_back("create_atomic");
        return 0x7000u;
    }
    std::uint32_t CreateFrame() override {
        calls.emplace_back("create_frame");
        const auto frame = next_frame;
        next_frame += 0x100u;
        return frame;
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x8000u || frame == 0x8100u,
              "both frames start from identity");
        calls.emplace_back("identity:" + std::to_string(frame));
    }
    void SetAtomicFrame(std::uint32_t atomic,
                        std::uint32_t frame) override {
        Check(atomic == 0x7000u && frame == 0x8000u,
              "atomic receives its own frame");
        words[atomic + 4u] = frame;
        calls.emplace_back("set_atomic_frame");
    }
    void SetAtomicGeometry(std::uint32_t atomic,
                           std::uint32_t geometry,
                           std::uint32_t flags) override {
        Check(atomic == 0x7000u && geometry == 0x2000u && flags == 0u,
              "atomic receives geometry without extra flags");
        calls.emplace_back("set_atomic_geometry");
    }
    void DestroyGeometry(std::uint32_t geometry) override {
        Check(geometry == 0x2000u, "geometry reference is released");
        calls.emplace_back("destroy_geometry");
    }
    std::uint32_t SharedAtomicCallback() override {
        calls.emplace_back("read_shared_callback");
        return shared_callback;
    }
    void SetSharedAtomicCallback(std::uint32_t callback) override {
        shared_callback = callback;
        calls.emplace_back("set_shared_callback");
    }
    std::uint32_t CreateClump() override {
        calls.emplace_back("create_clump");
        return 0x9000u;
    }
    void AddAtomic(std::uint32_t clump,
                   std::uint32_t atomic) override {
        Check(clump == 0x9000u && atomic == 0x7000u,
              "atomic is attached to created clump");
        calls.emplace_back("add_atomic");
    }
    void AddFrameChild(std::uint32_t parent,
                       std::uint32_t child) override {
        Check(parent == 0x8100u && child == 0x8000u,
              "atomic frame is parented under clump frame");
        calls.emplace_back("add_child");
    }
    int SetMaterialFlagsForAllAtomics(std::uint32_t clump,
                                      std::uint32_t flags) override {
        Check(clump == 0x9000u && flags == 0xffffffffu,
              "new clump gets all-one material flags");
        calls.emplace_back("set_material_flags");
        return 0;
    }
    void RotateFrame(std::uint32_t frame, pal4::GeoPlaneVector,
                     float, std::uint32_t) override {
        Check(frame == 0x8100u, "create rotates clump frame");
        calls.emplace_back("rotate_frame");
    }
    void ScaleFrame(std::uint32_t frame, pal4::GeoPlaneVector scale,
                    std::uint32_t combine) override {
        Check(frame == 0x8100u && scale.x == 1.f &&
              scale.y == 1.f && scale.z == 1.f && combine == 0u,
              "create initializes unit scale");
        calls.emplace_back("scale_frame");
    }
    void TranslateFrame(std::uint32_t frame,
                        pal4::GeoPlaneVector position,
                        std::uint32_t combine) override {
        Check(frame == 0x8100u && position.x == 0.f &&
              position.y == 0.f && position.z == 0.f && combine == 2u,
              "create initializes zero translation");
        calls.emplace_back("translate_frame");
    }
    int UpdateFrameObjects(std::uint32_t frame) override {
        saw_final_state = frame == 0x8100u;
        calls.emplace_back("update_transform");
        return 7;
    }
    float Float(std::uint32_t address) const {
        return std::bit_cast<float>(words.at(address));
    }
};

std::size_t CallIndex(const CreateRuntime& runtime,
                      const std::string& call) {
    for (std::size_t i = 0; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    return runtime.calls.size();
}

void CheckVertices(const GeometryRuntime& runtime,
                   std::array<std::uint32_t, 8u> values,
                   std::array<std::uint32_t, 8u> order,
                   const char* message) {
    Check(runtime.writes.size() == 8u, message);
    for (std::size_t i = 0; i < 8u; ++i)
        Check(runtime.writes[i] == std::pair<std::uint32_t, std::uint32_t>{
                  0x4000u + order[i] * 4u, values[order[i]]}, message);
    Check(runtime.calls.size() == 12u &&
              runtime.calls[0] == "read:8216" &&
              runtime.calls[1] == "lock" &&
              runtime.calls[2] == "read:12340" &&
              runtime.calls[11] == "unlock", message);
}

void TestMaterialFlags() {
    pal4::GeoPlaneState object;
    Runtime runtime;
    object.clump = 0x1234u;
    Check(pal4::GeoPlaneObject::SetMaterialFlags(
              object, 0xffffffffu, runtime) == -17 &&
              object.material_flags == 0xfeffffffu &&
              runtime.flags == 0xfeffffffu &&
              runtime.clump == 0x1234u && runtime.material_calls == 1,
          "all-ones high byte is changed to fe and forwarded");
    Check(pal4::GeoPlaneObject::SetMaterialFlags(
              object, 0xab345678u, runtime) == -17 &&
              object.material_flags == 0xab345678u &&
              runtime.flags == 0xab345678u &&
              runtime.material_calls == 2,
          "other high bytes are preserved");
    Check(pal4::GeoPlaneObject::SetMaterialFlags(
              object, 0u, runtime) == -17 &&
              object.material_flags == 0u && runtime.material_calls == 3,
          "zero material flags are still forwarded");
}

void TestVectors() {
    pal4::GeoPlaneState object;
    Runtime runtime;
    object.frame = 0x8100u;
    const pal4::GeoPlaneVector first{1.f, 2.f, 3.f};
    const pal4::GeoPlaneVector second{4.f, 5.f, 6.f};
    Check(pal4::GeoPlaneObject::SetRotation(
              object, first, false, runtime) == 29 &&
              object.rotation.x == 1.f && object.rotation.y == 2.f &&
              object.rotation.z == 3.f && object.scale.x == 0.f &&
              runtime.transform_calls == 1,
          "rotation replaces only the vector at +44");
    Check(pal4::GeoPlaneObject::SetRotation(
              object, second, true, runtime) == 29 &&
              object.rotation.x == 5.f && object.rotation.y == 7.f &&
              object.rotation.z == 9.f,
          "rotation additive mode updates all three components");
    Check(pal4::GeoPlaneObject::SetScale(
              object, first, false, runtime) == 29 &&
              object.scale.x == 1.f && object.scale.y == 2.f &&
              object.scale.z == 3.f && object.rotation.x == 5.f,
          "scale replaces only the vector at +32");
    Check(pal4::GeoPlaneObject::SetScale(
              object, second, true, runtime) == 29 &&
              object.scale.x == 5.f && object.scale.y == 7.f &&
              object.scale.z == 9.f,
          "scale additive mode updates all three components");
    Check(pal4::GeoPlaneObject::SetPosition(
              object, first, false, runtime) == 29 &&
              object.position.x == 1.f && object.position.y == 2.f &&
              object.position.z == 3.f && object.scale.x == 5.f,
          "position replaces only the vector at +56");
    Check(pal4::GeoPlaneObject::SetPosition(
              object, second, true, runtime) == 29 &&
              object.position.x == 5.f && object.position.y == 7.f &&
              object.position.z == 9.f && runtime.transform_calls == 6,
          "position additive mode updates frame after every setter");
}

void TestFrameTransform() {
    pal4::GeoPlaneState object;
    Runtime runtime;
    Check(pal4::GeoPlaneObject::UpdateFrameTransform(
              object, runtime) == 0 &&
              runtime.transform_events.empty(),
          "null frame skips transform calls");
    object.frame = 0x8100u;
    object.scale = {2.f, 3.f, 4.f};
    object.rotation = {10.f, 20.f, 30.f};
    object.position = {5.f, 6.f, 7.f};
    Check(pal4::GeoPlaneObject::UpdateFrameTransform(
              object, runtime) == 29 &&
              runtime.transform_events == std::vector<std::string>{
                  "identity", "scale:0", "rotate:x:10:2",
                  "rotate:y:20:2", "rotate:z:30:2",
                  "translate:2", "update"} &&
              runtime.last_scale.x == 2.f &&
              runtime.last_scale.y == 3.f &&
              runtime.last_scale.z == 4.f &&
              runtime.last_position.x == 5.f &&
              runtime.last_position.y == 6.f &&
              runtime.last_position.z == 7.f,
          "default mode scales before XYZ rotations and translation");
    runtime.transform_events.clear();
    object.rotation_first = 1u;
    Check(pal4::GeoPlaneObject::UpdateFrameTransform(
              object, runtime) == 29 &&
              runtime.transform_events == std::vector<std::string>{
                  "identity", "rotate:x:10:0", "rotate:y:20:2",
                  "rotate:z:30:2", "scale:2", "translate:2",
                  "update"},
          "rotation-first mode changes first combine and scale ordering");
}

void TestGeometryVertices() {
    pal4::GeoPlaneState object;
    GeometryRuntime runtime;
    object.atomic = 0x2000u;
    runtime.words[0x2018u] = 0x3000u;
    runtime.words[0x3034u] = 0x4000u;
    constexpr std::array<std::uint32_t, 8u> forward{
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u};
    constexpr std::array<std::uint32_t, 8u> swapped{
        1u, 0u, 3u, 2u, 5u, 4u, 7u, 6u};

    Check(pal4::GeoPlaneObject::UpdateGeometryVertices(
              object, {1u, 2u, 3u, 4u}, runtime) == 0x193 &&
              object.vertex_bounds ==
                  std::array<std::uint32_t, 4u>{1u, 2u, 3u, 4u},
          "geometry update caches all four input words");
    CheckVertices(runtime, {1u, 2u, 1u, 4u, 3u, 2u, 3u, 4u},
                  forward, "normal vertex layout and lock order");

    runtime.ClearCalls();
    Check(pal4::GeoPlaneObject::SetVertexOrientation(
              object, 1u, runtime) == 0x93u &&
              object.reversed_vertices == 1u,
          "orientation change returns low byte of unlock result");
    CheckVertices(runtime, {2u, 1u, 4u, 1u, 2u, 3u, 4u, 3u},
                  swapped, "conditional reversed layout and write order");

    runtime.ClearCalls();
    Check(pal4::GeoPlaneObject::SetVertexOrientation(
              object, 1u, runtime) == 1u && runtime.calls.empty(),
          "unchanged orientation skips geometry operations");

    Check(pal4::GeoPlaneObject::UpdateGeometryVertices(
              object, {5u, 6u, 7u, 8u}, runtime) == 0x193,
          "direct update runs even when orientation is unchanged");
    CheckVertices(runtime, {6u, 5u, 6u, 7u, 8u, 5u, 8u, 7u},
                  forward, "direct reversed layout differs from conditional");

    runtime.ClearCalls();
    Check(pal4::GeoPlaneObject::SetVertexOrientation(
              object, 0u, runtime) == 0x93u &&
              object.reversed_vertices == 0u,
          "normal orientation change invokes geometry update");
    CheckVertices(runtime, {5u, 6u, 5u, 8u, 7u, 6u, 7u, 8u},
                  forward, "normal layout restores cached bounds");
}

void TestGeometryMode() {
    pal4::GeoPlaneState object;
    Check(pal4::GeoPlaneObject::SetGeometryMode(object, 1) == 1 &&
              object.geometry_state == 3u,
          "mode one maps to state three");
    Check(pal4::GeoPlaneObject::SetGeometryMode(object, 2) == 2 &&
              object.geometry_state == 2u,
          "mode two is retained");
    Check(pal4::GeoPlaneObject::SetGeometryMode(object, 0) == 0 &&
              object.geometry_state == 1u,
          "mode zero maps to state one");
    Check(pal4::GeoPlaneObject::SetGeometryMode(object, -1) == -1 &&
              object.geometry_state == 1u &&
              pal4::GeoPlaneObject::SetGeometryMode(object, 3) == 3 &&
              object.geometry_state == 1u,
          "out-of-range modes map to state one and return input");
}

void TestCreateCenteredPlane() {
    pal4::GeoPlaneState object;
    CreateRuntime runtime;
    object.active = 1u;
    object.geometry_state = 2u;
    Check(pal4::GeoPlaneObject::Create(
              object, 0xabcdu, {4.f, 6.f}, 0x5au, 1u, runtime) == 1u &&
              runtime.saw_final_state &&
              object.width == 4.f && object.height == 6.f &&
              object.material_flags == 0xffffffffu &&
              object.geometry_state == 2u && object.option == 0x5au &&
              object.rotation.x == 0.f && object.position.x == 0.f &&
              object.vertex_bounds ==
                  std::array<std::uint32_t, 4u>{
                      0u, 0u, 0x3f800000u, 0x3f800000u},
          "create resets transform and cached vertex bounds after assembly");
    Check(runtime.Float(0x5000u) == -2.f &&
              runtime.Float(0x5004u) == 3.f &&
              runtime.Float(0x5008u) == 0.f &&
              runtime.Float(0x500cu) == -2.f &&
              runtime.Float(0x5010u) == -3.f &&
              runtime.Float(0x5018u) == 2.f &&
              runtime.Float(0x501cu) == 3.f &&
              runtime.Float(0x5024u) == 2.f &&
              runtime.Float(0x5028u) == -3.f,
          "centered plane has four positions and zero depth");
    Check(runtime.Float(0x6000u) == 0.f &&
              runtime.Float(0x6004u) == 0.f &&
              runtime.Float(0x600cu) == 1.f &&
              runtime.Float(0x6010u) == 1.f &&
              runtime.Float(0x6014u) == 0.f &&
              runtime.Float(0x6018u) == 1.f &&
              runtime.Float(0x601cu) == 1.f,
          "plane stores the four native UV pairs");
    for (std::uint32_t address = 0x6100u; address < 0x6110u; ++address)
        Check(runtime.bytes.at(address) == 0xffu,
              "all four vertex colors are opaque white");
    Check(runtime.words[0x4004u] == 11u &&
              runtime.words[0x4008u] == 22u &&
              runtime.words[0x400cu] == 33u &&
              runtime.words[0x4010u] == 44u &&
              runtime.words[0x9004u] == 0x8100u &&
              runtime.shared_callback == 0xf00du,
          "sphere, clump frame and shared atomic callback are stored");
    Check(CallIndex(runtime, "get_world") == 0u &&
              CallIndex(runtime, "create_material") <
                  CallIndex(runtime, "set_texture") &&
              CallIndex(runtime, "set_texture") <
                  CallIndex(runtime, "create_geometry") &&
              CallIndex(runtime, "triangle:12288") <
                  CallIndex(runtime, "triangle_material:12288") &&
              CallIndex(runtime, "triangle_material:12296") <
                  CallIndex(runtime, "destroy_material") &&
              CallIndex(runtime, "destroy_material") <
                  CallIndex(runtime, "sphere") &&
              CallIndex(runtime, "sphere") <
                  CallIndex(runtime, "unlock_geometry") &&
              CallIndex(runtime, "unlock_geometry") <
                  CallIndex(runtime, "create_atomic") &&
              CallIndex(runtime, "set_atomic_geometry") <
                  CallIndex(runtime, "destroy_geometry") &&
              CallIndex(runtime, "destroy_geometry") <
                  CallIndex(runtime, "create_clump") &&
              CallIndex(runtime, "set_material_flags") <
                  CallIndex(runtime, "update_transform"),
          "native resource and assembly stages remain ordered");
}

void TestCreateUncenteredAndFailure() {
    pal4::GeoPlaneState object;
    CreateRuntime runtime;
    runtime.shared_callback = 0xabcdu;
    Check(pal4::GeoPlaneObject::Create(
              object, 0u, {2.f, 6.f}, 0u, 0u, runtime) == 1u &&
              runtime.Float(0x5000u) == -1.f &&
              runtime.Float(0x5004u) == 6.f &&
              runtime.Float(0x5010u) == 0.f &&
              runtime.Float(0x5028u) == 0.f &&
              runtime.shared_callback == 0xabcdu &&
              CallIndex(runtime, "set_texture") == runtime.calls.size() &&
              CallIndex(runtime, "set_shared_callback") ==
                  runtime.calls.size(),
          "zero mode uses top-to-zero plane without texture or callback overwrite");

    pal4::GeoPlaneState failed;
    CreateRuntime failure_runtime;
    failed.active = 1u;
    failure_runtime.geometry_failure = true;
    bool thrown = false;
    try {
        (void)pal4::GeoPlaneObject::Create(
            failed, 0u, {1.f, 1.f}, 0u, 0u, failure_runtime);
    } catch (const std::runtime_error& error) {
        thrown = std::string(error.what()) ==
                 "create RpGeometry failed in geoPlane::create!";
    }
    Check(thrown && failed.active == 0u &&
              failure_runtime.calls == std::vector<std::string>{
                  "get_world", "create_material", "create_geometry"},
          "geometry creation failure throws native message before assembly");
}

struct DestroyRuntime final : pal4::IGeoPlaneDestroyRuntime {
    std::vector<std::string> calls;
    std::uint32_t world{};
    std::uint32_t ClumpWorld(std::uint32_t clump) override {
        Check(clump == 0x9000u || clump == 0u,
              "destroy queries world for current clump");
        calls.emplace_back("get_world");
        return world;
    }
    void RemoveClumpFromWorld(std::uint32_t input_world,
                              std::uint32_t clump) override {
        Check(input_world == world && clump == 0x9000u,
              "world removal uses current clump");
        calls.emplace_back("remove_world");
    }
    void ClearClumpFrame(std::uint32_t clump) override {
        Check(clump == 0x9000u, "clump frame link is cleared");
        calls.emplace_back("clear_clump_frame");
    }
    void DestroyClump(std::uint32_t clump) override {
        Check(clump == 0x9000u, "clump is destroyed once");
        calls.emplace_back("destroy_clump");
    }
    void DestroyFrame(std::uint32_t frame) override {
        Check(frame == 0x8100u, "parent frame is destroyed once");
        calls.emplace_back("destroy_frame");
    }
};

struct RenderRuntime final : pal4::IGeoPlaneRenderRuntime {
    std::vector<std::string> calls;
    std::uint32_t GetRenderState(std::uint32_t state) override {
        calls.push_back("get:" + std::to_string(state));
        return state + 100u;
    }
    void SetRenderState(std::uint32_t state,
                        std::uint32_t value) override {
        calls.push_back("set:" + std::to_string(state) + ":" +
                        std::to_string(value));
    }
    std::uint32_t GetDeviceState(std::uint32_t state) override {
        calls.push_back("device_get:" + std::to_string(state));
        return 0x777u;
    }
    void SetDeviceState(std::uint32_t state,
                        std::uint32_t value) override {
        calls.push_back("device_set:" + std::to_string(state) + ":" +
                        std::to_string(value));
    }
    void RenderClump(std::uint32_t clump) override {
        calls.push_back("render:" + std::to_string(clump));
    }
    void SetTextureStage(std::uint32_t first,
                         std::uint32_t second,
                         std::uint32_t texture,
                         std::uint32_t stage) override {
        calls.push_back("stage:" + std::to_string(first) + ":" +
                        std::to_string(second) + ":" +
                        std::to_string(texture) + ":" +
                        std::to_string(stage));
    }
    int ClearStreamSource(std::uint32_t stream) override {
        calls.push_back("stream:" + std::to_string(stream));
        return stream == 1u ? -23 : 0;
    }
};

void TestDestroy() {
    pal4::GeoPlaneState object;
    DestroyRuntime runtime;
    object.clump = 0x9000u;
    object.atomic = 0x7000u;
    object.frame = 0x8100u;
    object.rotation_first = 1u;
    pal4::GeoPlaneObject::Destroy(object, runtime);
    Check(runtime.calls.empty() && object.clump == 0x9000u &&
              object.frame == 0x8100u,
          "inactive plane is untouched");

    object.active = 1u;
    runtime.world = 0xaaaau;
    pal4::GeoPlaneObject::Destroy(object, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "get_world", "remove_world", "clear_clump_frame",
              "destroy_clump", "destroy_frame"} &&
              object.active == 0u && object.clump == 0u &&
              object.atomic == 0u && object.frame == 0u &&
              object.rotation_first == 0u,
          "world-owned plane releases resources in native order");

    runtime.calls.clear();
    object.active = 1u;
    object.clump = 0x9000u;
    object.atomic = 0x7000u;
    object.frame = 0x8100u;
    runtime.world = 0u;
    pal4::GeoPlaneObject::Destroy(object, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "get_world", "clear_clump_frame", "destroy_clump",
              "destroy_frame"},
          "unowned plane omits world removal");

    runtime.calls.clear();
    object.active = 1u;
    object.atomic = 0x7000u;
    object.rotation_first = 1u;
    pal4::GeoPlaneObject::Destroy(object, runtime);
    Check(runtime.calls == std::vector<std::string>{"get_world"} &&
              object.atomic == 0u && object.rotation_first == 0u,
          "active object with zero handles still clears logical state");
}

void TestRender() {
    pal4::GeoPlaneState object;
    RenderRuntime runtime;
    Check(pal4::GeoPlaneObject::Render(
              object, 0x11u, 0x22u, runtime) == 0 &&
              runtime.calls.empty(),
          "inactive plane skips D3D state changes");
    object.active = 1u;
    object.option = 1u;
    object.geometry_state = 3u;
    object.clump = 0x9000u;
    Check(pal4::GeoPlaneObject::Render(
              object, 0x11u, 0x22u, runtime) == -23 &&
              runtime.calls == std::vector<std::string>{
                  "get:27", "get:19", "get:20",
                  "set:27:1", "set:19:5", "set:20:2",
                  "get:7", "get:14", "set:7:1", "set:14:0",
                  "device_get:20", "device_set:20:3", "render:36864",
                  "device_set:20:1911", "device_set:1:0",
                  "set:7:107", "set:14:114", "set:27:127",
                  "set:19:119", "set:20:120",
                  "stage:17:34:0:0", "stage:17:34:0:1",
                  "stream:0", "stream:1"},
          "active render follows native save, override and restore order");
    runtime.calls.clear();
    object.option = 0u;
    (void)pal4::GeoPlaneObject::Render(object, 0u, 0u, runtime);
    Check(runtime.calls[5u] == "set:20:6" &&
              runtime.calls[11u] == "device_set:20:3" &&
              runtime.calls.back() == "stream:1",
          "zero option selects render-state value six");
}

}  // namespace

int main() {
    TestMaterialFlags();
    TestVectors();
    TestFrameTransform();
    TestGeometryVertices();
    TestGeometryMode();
    TestCreateCenteredPlane();
    TestCreateUncenteredAndFailure();
    TestDestroy();
    TestRender();
}
