#include "pal4/effect_visual_object.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectVisualObjectRuntime {
    std::uint32_t allocation = 0x5000u;
    bool initialize_result = true;
    std::unordered_map<std::uint32_t, std::uint32_t> geometry_words{
        {0x5010u, 0x9000u}};
    std::vector<std::string> calls;
    pal4::EffectMatrix applied{};
    std::array<std::uint32_t, 3u> last_scale{};

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void WriteGeometryByte(std::uint32_t address,
                           std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address) + ":" +
                        std::to_string(value));
    }
    void WriteGeometryWord(std::uint32_t address,
                           std::uint32_t value) override {
        calls.push_back("word:" + std::to_string(address) + ":" +
                        std::to_string(value));
        if (address != 0x5010u) geometry_words[address] = value;
    }
    std::uint32_t ReadGeometryWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return geometry_words.at(address);
    }
    bool InitializeGeometry(std::uint32_t geometry,
                            std::uint32_t source, std::uint32_t vector,
                            std::uint8_t mode,
                            std::uint32_t option) override {
        Check(geometry == allocation && source == 0x1111u &&
                  vector == 0x2222u && mode == 3u && option == 0x3333u,
              "geometry initialization receives exact arguments");
        calls.emplace_back("init_geometry");
        return initialize_result;
    }
    void DestroyGeometry(std::uint32_t geometry) override {
        calls.push_back("destroy_geometry:" + std::to_string(geometry));
    }
    std::uint32_t CreateFrame() override {
        calls.emplace_back("create_frame");
        return 0x6000u;
    }
    void DestroyFrame(std::uint32_t frame) override {
        calls.push_back("destroy_frame:" + std::to_string(frame));
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        calls.push_back("identity:" + std::to_string(frame));
    }
    void ScaleFrame(
        std::uint32_t frame,
        const std::array<std::uint32_t, 3u>& scale,
        std::uint32_t mode) override {
        Check(scale[2] == 0x3f800000u, "frame Z scale is one");
        last_scale = scale;
        calls.push_back("scale:" + std::to_string(frame) + ":" +
                        std::to_string(mode));
    }
    void RotateFrame(std::uint32_t frame, std::uint32_t axis,
                     float angle, std::uint32_t mode) override {
        Check(axis == 0x84b35cu || axis == 0x84b368u ||
                  axis == 0x84b374u,
              "rotation axis is one of three IDA vectors");
        calls.push_back("rotate:" + std::to_string(frame) + ":" +
                        std::to_string(axis) + ":" +
                        std::to_string(angle) + ":" +
                        std::to_string(mode));
    }
    void TranslateFrame(
        std::uint32_t frame,
        const std::array<std::uint32_t, 3u>& translation,
        std::uint32_t mode) override {
        Check(translation == std::array<std::uint32_t, 3u>{2u, 3u, 4u},
              "initial transform translates from object +28");
        calls.push_back("translate:" + std::to_string(frame) + ":" +
                        std::to_string(mode));
    }
    std::uint8_t UpdateFrameObjects(std::uint32_t frame) override {
        calls.push_back("update_frame:" + std::to_string(frame));
        return frame == 0x9000u ? 11u : 14u;
    }
    void AddFrameChild(std::uint32_t frame,
                       std::uint32_t child) override {
        Check(frame == 0x6000u && child == 0x9000u,
              "RenderWare child comes from geometry +16");
        calls.emplace_back("add_child");
    }
    std::int32_t SetGeometryState(std::uint32_t geometry,
                                  std::int32_t mode) override {
        Check(geometry == allocation,
              "state update targets current geometry");
        calls.push_back("state:" + std::to_string(mode));
        return mode + 7;
    }
    void NoopUpdate(std::uint32_t first,
                    std::uint32_t second) override {
        Check(first == 0x1111u && second == 0x2222u,
              "native no-op receives first two update args");
        calls.emplace_back("noop");
    }
    std::uint32_t ReadObjectWord(std::uint32_t address) override {
        Check(address == 0x3337u, "simple transform reads input +4");
        calls.emplace_back("read_object");
        return 0x7000u;
    }
    pal4::EffectMatrix GetObjectTransform(
        std::uint32_t object) override {
        calls.push_back("get_transform:" + std::to_string(object));
        pal4::EffectMatrix matrix{};
        matrix.up = {0.0f, 1.0f, 0.0f};
        matrix.position = object == 0x7000u
                              ? pal4::EffectVector3{0.0f, 0.0f, 0.0f}
                              : pal4::EffectVector3{0.0f, 0.0f, 1.0f};
        return matrix;
    }
    void Normalize(pal4::EffectVector3& vector) override {
        calls.emplace_back("normalize");
        const float length = std::sqrt(vector.x * vector.x +
                                       vector.y * vector.y +
                                       vector.z * vector.z);
        Check(length > 0.0f, "test vector is nonzero");
        vector.x /= length;
        vector.y /= length;
        vector.z /= length;
    }
    pal4::EffectMatrix SetupParentTransform(
        std::uint32_t frame) override {
        Check(frame == 0x6000u, "parent transform uses object frame");
        calls.emplace_back("setup_parent");
        pal4::EffectMatrix matrix{};
        matrix.right = {1.0f, 0.0f, 0.0f};
        matrix.up = {0.0f, 1.0f, 0.0f};
        matrix.at = {0.0f, 0.0f, 1.0f};
        return matrix;
    }
    void TransformVector(pal4::EffectVector3& vector,
                         const pal4::EffectMatrix& matrix) override {
        Check(matrix.right.x == 1.0f, "parent matrix is passed to vector transform");
        calls.emplace_back("transform_vector");
        (void)vector;
    }
    void UpdateMatrix(pal4::EffectMatrix& matrix) override {
        Check(matrix.flags == 0x20003u &&
                  matrix.position.x == 0.0f &&
                  matrix.position.y == 0.0f &&
                  matrix.position.z == 0.0f,
              "orientation matrix flags and translation");
        calls.emplace_back("update_matrix");
    }
    void ApplyMatrix(std::uint32_t frame,
                     const pal4::EffectMatrix& matrix,
                     std::uint32_t mode) override {
        Check(frame == 0x9000u && mode == 2u,
              "orientation applies to child frame with mode 2");
        applied = matrix;
        calls.emplace_back("apply_matrix");
    }
    std::uint8_t RenderWithStates(std::uint32_t geometry,
                                  std::uint32_t first,
                                  std::uint32_t second) override {
        Check(geometry == 0x5000u && first == 0x1111u &&
                  second == 0x2222u,
              "render forwards geometry and native register arguments");
        calls.emplace_back("render");
        return 17u;
    }
};

void TestEnabledAndUpdate() {
    Runtime runtime;
    pal4::EffectVisualObjectState object;
    object.geometry = runtime.allocation;
    Check(pal4::EffectVisualObject::SetEnabled(object, 0u, runtime) == 9 &&
              object.enabled == 0u &&
              runtime.calls == std::vector<std::string>{"state:2"},
          "disabled object with no geometry mode selects state 2");
    object.geometry_mode = 1u;
    Check(pal4::EffectVisualObject::SetEnabled(object, 0u, runtime) == 7 &&
              pal4::EffectVisualObject::SetEnabled(object, 1u, runtime) == 7 &&
              object.enabled == 1u,
          "geometry mode and enabled flag each select state 0");

    runtime.calls.clear();
    Check(pal4::EffectVisualObject::Update(
              object, 0x1111u, 0x2222u, 0x3333u, runtime) == 0u &&
              runtime.calls.empty(),
          "inactive object skips every update service");
    object.active = 1u;
    Check(pal4::EffectVisualObject::Update(
              object, 0x1111u, 0x2222u, 0x3333u, runtime) == 11u &&
              runtime.calls == std::vector<std::string>{
                  "noop", "read:20496", "identity:36864",
                  "rotate:36864:8696692:0.000000:0", "scale:36864:2",
                  "update_frame:36864"},
          "geometry mode dispatches rotation after no-op");
    runtime.calls.clear();
    object.geometry_mode = 0u;
    object.transform_mode = 1u;
    object.frame = 0x6000u;
    object.property_flag = 1u;
    Check(pal4::EffectVisualObject::Update(
              object, 0x1111u, 0x2222u, 0x3333u, runtime) == 11u &&
              runtime.calls.front() == "noop" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "read_object") != runtime.calls.end() &&
              runtime.calls[runtime.calls.size() - 2u] == "apply_matrix",
          "transform mode dispatches simple transform");
    runtime.calls.clear();
    object.transform_mode = 0u;
    object.owner = std::bit_cast<std::uint32_t>(0.0f);
    object.source = std::bit_cast<std::uint32_t>(0.0f);
    object.context = std::bit_cast<std::uint32_t>(2.0f);
    Check(pal4::EffectVisualObject::Update(
              object, 0x1111u, 0x2222u, 0x3333u, runtime) == 11u &&
              runtime.calls.front() == "noop" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "read_object") == runtime.calls.end() &&
              runtime.calls[runtime.calls.size() - 2u] == "apply_matrix",
          "remaining active mode dispatches complex transform");

    runtime.calls.clear();
    object.active = 0u;
    Check(pal4::EffectVisualObject::InitializeTransform(object, runtime) ==
              0u && runtime.calls.empty(),
          "inactive frame transform returns before RenderWare calls");
    object.active = 1u;
    object.frame = 0x6000u;
    object.translation[1] = 2u;
    object.translation[2] = 3u;
    object.translation[3] = 4u;
    Check(pal4::EffectVisualObject::InitializeTransform(object, runtime) ==
              14u &&
              runtime.calls == std::vector<std::string>{
                  "identity:24576", "scale:24576:0",
                  "rotate:24576:8696668:0.000000:2",
                  "rotate:24576:8696680:0.000000:2",
                  "rotate:24576:8696692:0.000000:2",
                  "translate:24576:2", "update_frame:24576"},
          "active transform applies identity, scale, XYZ rotations and translation");
}

void TestInitializeAndDestroy() {
    Runtime runtime;
    pal4::EffectVisualObjectState object;
    object.geometry_mode = 1u;
    Check(pal4::EffectVisualObject::Initialize(
              object, 0x1111u, 0x2222u, 3u, 0xa5u, 0x3333u,
              runtime) == 1u &&
              object.active == 1u && object.transform_mode == 1u &&
              object.property_flag == 0xa5u &&
              object.frame == 0x6000u && object.geometry == 0x5000u &&
              object.translation[0] == 0u &&
              object.scale[0] == 0x3f800000u &&
              object.scale[1] == 0x3f800000u,
          "new visual object stores native layout and initialized scale");
    Check(runtime.calls == std::vector<std::string>{
              "allocate:84", "byte:20480:0", "byte:20481:0",
              "byte:20482:0", "word:20484:3", "word:20488:0",
              "word:20492:0", "word:20496:0", "init_geometry",
              "create_frame", "read:20496",
              "add_child", "state:0"},
          "property initializer follows IDA allocation and frame order");
    runtime.calls.clear();
    pal4::EffectVisualObject::Deinitialize(object, runtime);
    Check(object.active == 0u && object.transform_mode == 1u &&
              object.property_flag == 1u && object.frame == 0u &&
              object.geometry == 0u && object.scale[0] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "destroy_frame:24576", "destroy_geometry:20480",
                  "free:20480"},
          "deinitialize destroys frame before geometry and resets flags");
    runtime.calls.clear();
    pal4::EffectVisualObject::Deinitialize(object, runtime);
    Check(runtime.calls.empty(), "inactive deinitialize is a no-op");
}

void TestInitializeFromObject() {
    Runtime runtime;
    pal4::EffectVisualObjectState object;
    object.geometry_mode = 0u;
    pal4::EffectVisualObjectState source;
    source.owner = 10u;
    source.source = 20u;
    source.context = 30u;
    Check(pal4::EffectVisualObject::InitializeFromObject(
              object, source, 0x1111u, 0x2222u, 3u,
              0x5au, 0x3333u, runtime) == 1u &&
              object.owner == 10u && object.source == 20u &&
              object.context == 30u && object.transform_mode == 0u &&
              object.property_flag == 0x5au,
          "template initializer copies the first three words");
    const auto state_it = std::find(runtime.calls.begin(),
                                    runtime.calls.end(), "state:2");
    const auto frame_it = std::find(runtime.calls.begin(),
                                    runtime.calls.end(), "create_frame");
    Check(state_it != runtime.calls.end() && frame_it != runtime.calls.end() &&
              state_it < frame_it,
          "template initializer sets geometry state before creating frame");
}

void TestInitializationFailure() {
    Runtime runtime;
    runtime.initialize_result = false;
    pal4::EffectVisualObjectState object;
    object.active = 0u;
    Check(pal4::EffectVisualObject::Initialize(
              object, 0x1111u, 0x2222u, 3u, 1u, 0x3333u,
              runtime) == 0u &&
              object.geometry == 0u && object.frame == 0u &&
              runtime.calls.back() == "free:20480" &&
              runtime.calls[runtime.calls.size() - 2u] ==
                  "destroy_geometry:20480",
          "failed geometry initialization cleans allocated geometry");
    Runtime allocation_failure;
    allocation_failure.allocation = 0u;
    allocation_failure.initialize_result = false;
    Check(pal4::EffectVisualObject::Initialize(
              object, 0x1111u, 0x2222u, 3u, 1u, 0x3333u,
              allocation_failure) == 0u &&
              object.geometry == 0u &&
              allocation_failure.calls == std::vector<std::string>{
                  "allocate:84", "init_geometry"},
          "null allocation is forwarded to geometry initializer without free");
}

void TestTransformBranches() {
    Runtime runtime;
    pal4::EffectVisualObjectState object;
    object.active = 1u;
    object.frame = 0x6000u;
    object.geometry = 0x5000u;
    object.scale[0] = std::bit_cast<std::uint32_t>(2.0f);
    object.scale[1] = std::bit_cast<std::uint32_t>(3.0f);
    object.translation[0] = std::bit_cast<std::uint32_t>(30.0f);
    object.owner = std::bit_cast<std::uint32_t>(0.0f);
    object.source = std::bit_cast<std::uint32_t>(0.0f);
    object.context = std::bit_cast<std::uint32_t>(2.0f);

    for (int complex = 0; complex != 2; ++complex) {
        for (int property = 0; property != 2; ++property) {
            for (int enabled = 0; enabled != 2; ++enabled) {
                runtime.calls.clear();
                object.property_flag = static_cast<std::uint8_t>(property);
                object.enabled = static_cast<std::uint8_t>(enabled);
                const auto result = complex != 0
                    ? pal4::EffectVisualObject::UpdateTransformComplex(
                          object, runtime)
                    : pal4::EffectVisualObject::UpdateTransform(
                          object, 0x3333u, runtime);
                Check(result == 11u &&
                          runtime.last_scale ==
                              std::array<std::uint32_t, 3u>{
                                  object.scale[0], object.scale[1],
                                  0x3f800000u},
                      "all transform branches scale and update child frame");
                const auto count = [&](const std::string& call) {
                    return std::count(runtime.calls.begin(),
                                      runtime.calls.end(), call);
                };
                Check(count("transform_vector") == 3 &&
                          count("normalize") ==
                              (complex != 0 ? (property != 0 ? 8 : 9)
                                            : (property != 0 ? 5 : 9)) &&
                          count("update_matrix") == 1 &&
                          count("apply_matrix") == 1,
                      "native vector and matrix call counts match each branch");
                Check(runtime.calls.back() == "update_frame:36864" &&
                          runtime.calls[runtime.calls.size() - 2u] ==
                              "apply_matrix" &&
                          runtime.calls[runtime.calls.size() - 3u].starts_with(
                              "scale:") == (complex == 0),
                      "matrix application follows branch-specific frame operations");
                const auto length_squared = [](pal4::EffectVector3 v) {
                    return v.x * v.x + v.y * v.y + v.z * v.z;
                };
                Check(runtime.applied.flags == 0x20003u &&
                          std::abs(length_squared(runtime.applied.right) - 1.0f) < 0.001f &&
                          std::abs(length_squared(runtime.applied.up) - 1.0f) < 0.001f &&
                          std::abs(length_squared(runtime.applied.at) - 1.0f) < 0.001f,
                      "computed orientation contains normalized coordinate axes");
                const auto expected_right = pal4::EffectVector3{-1.0f, 0.0f, 0.0f};
                const auto expected_up = pal4::EffectVector3{0.0f, 1.0f, 0.0f};
                Check(runtime.applied.right.x == expected_right.x &&
                          runtime.applied.right.y == expected_right.y &&
                          runtime.applied.right.z == expected_right.z &&
                          runtime.applied.up.x == expected_up.x &&
                          runtime.applied.up.y == expected_up.y &&
                          runtime.applied.up.z == expected_up.z,
                      "orientation direction follows IDA property branch");
                Check(count("rotate:36864:8696668:-90.000000:0") ==
                          (complex == 0 && enabled != 0 ? 1 : 0) &&
                          count("rotate:36864:8696668:-90.000000:2") ==
                          (complex != 0 && enabled != 0 ? 1 : 0),
                      "optional X rotation uses native mode and order");
                Check(count("rotate:36864:8696692:30.000000:0") ==
                          (complex == 0 && enabled == 0 ? 1 : 0) &&
                          count("rotate:36864:8696692:30.000000:2") ==
                          (complex != 0 || enabled != 0 ? 1 : 0) &&
                          count("scale:36864:0") == (complex != 0 ? 1 : 0) &&
                          count("scale:36864:2") == (complex == 0 ? 1 : 0),
                      "Z rotation and scale modes match native path");
            }
        }
    }
    object.active = 0u;
    runtime.calls.clear();
    Check(pal4::EffectVisualObject::UpdateTransform(
              object, 0x3333u, runtime) == 0u &&
              pal4::EffectVisualObject::UpdateTransformComplex(
                  object, runtime) == 0u && runtime.calls.empty(),
          "inactive transform methods do not touch native services");
    Check(pal4::EffectVisualObject::Render(
              object, 0x1111u, 0x2222u, runtime) == 0u &&
              runtime.calls.empty(),
          "inactive render returns without forwarding");
    object.active = 1u;
    Check(pal4::EffectVisualObject::Render(
              object, 0x1111u, 0x2222u, runtime) == 17u &&
              runtime.calls == std::vector<std::string>{"render"},
          "active render forwards both arguments and return byte");
}

}  // namespace

int main() {
    TestEnabledAndUpdate();
    TestInitializeAndDestroy();
    TestInitializeFromObject();
    TestInitializationFailure();
    TestTransformBranches();
}
