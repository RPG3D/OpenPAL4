#include "pal4/visual_model.h"

#include <array>
#include <cstdlib>
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

struct Runtime final : pal4::IVisualModelRuntime {
    int frame_calls{};
    int material_calls{};
    std::uint32_t clump{};
    std::uint32_t flags{};
    std::vector<std::string> frame_order;

    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x4000u, "identity targets model frame");
        frame_order.emplace_back("identity");
    }
    void ScaleFrame(std::uint32_t frame,
                    pal4::VisualModelVector scale,
                    std::uint32_t mode) override {
        Check(frame == 0x4000u && mode == 0u &&
                  scale.x != 1234.0f,
              "scale uses model field and mode zero");
        frame_order.emplace_back("scale");
    }
    void RotateFrame(std::uint32_t frame,
                     std::uint32_t axis, float angle,
                     std::uint32_t mode) override {
        Check(frame == 0x4000u && mode == 2u &&
                  (axis == 0x84b380u || axis == 0x84b38cu ||
                   axis == 0x84b398u) && angle >= 0.0f,
              "rotation uses verified XYZ axes and model values");
        frame_order.push_back(axis == 0x84b380u ? "rotate_x" :
                              axis == 0x84b38cu ? "rotate_y" : "rotate_z");
    }
    void TranslateFrame(std::uint32_t frame,
                        pal4::VisualModelVector position,
                        std::uint32_t mode) override {
        Check(frame == 0x4000u && mode == 2u && position.z >= 0.0f,
              "translation uses model position and mode two");
        frame_order.emplace_back("translate");
    }
    int UpdateFrameObjects(std::uint32_t frame) override {
        Check(frame == 0x4000u, "frame update targets model frame");
        frame_order.emplace_back("update");
        ++frame_calls;
        return -frame_calls;
    }
    int SetMaterialFlagsForAllAtomics(std::uint32_t input_clump,
                                       std::uint32_t input_flags) override {
        ++material_calls;
        clump = input_clump;
        flags = input_flags;
        return 37;
    }
};

void TestVectors() {
    Runtime runtime;
    pal4::VisualModelState model;
    model.clump = 0x1234u;
    model.frame = 0x4000u;
    using Setter = int (*)(pal4::VisualModelState&,
                           pal4::VisualModelVector, bool,
                           pal4::IVisualModelRuntime&);
    const std::array<Setter, 3> setters{
        &pal4::VisualModel::SetRotation,
        &pal4::VisualModel::SetScale,
        &pal4::VisualModel::SetPosition};
    const std::array<pal4::VisualModelVector*, 3> targets{
        &model.rotation, &model.scale, &model.position};
    for (std::size_t index = 0; index < setters.size(); ++index) {
        Check(setters[index](model, {2.0f, 3.0f, 4.0f}, false,
                             runtime) == -static_cast<int>(index * 3u + 1u),
              "absolute setter forwards frame update result");
        Check(targets[index]->x == 2.0f && targets[index]->y == 3.0f &&
                  targets[index]->z == 4.0f,
              "absolute setter replaces all three components");
        Check(setters[index](model, {-1.0f, 5.0f, 0.5f}, true,
                             runtime) == -static_cast<int>(index * 3u + 2u),
              "additive setter forwards frame update result");
        Check(targets[index]->x == 1.0f && targets[index]->y == 8.0f &&
                  targets[index]->z == 4.5f,
              "additive setter adds all three components");
        Check(setters[index](model, {0.0f, 0.0f, 0.0f}, true,
                             runtime) == -static_cast<int>(index * 3u + 3u),
              "zero delta still updates frame");
    }
    Check(model.rotation.x == 1.0f && model.scale.y == 8.0f &&
              model.position.z == 4.5f && runtime.frame_calls == 9 &&
              runtime.material_calls == 0,
          "setters leave other groups intact and update once per call");
    const std::vector<std::string> expected{
        "identity", "scale", "rotate_x", "rotate_y", "rotate_z",
        "translate", "update"};
    Check(runtime.frame_order.size() == expected.size() * 9u,
          "every setter runs all frame operations");
    for (std::size_t index = 0; index < runtime.frame_order.size(); ++index)
        Check(runtime.frame_order[index] == expected[index % expected.size()],
              "frame transform preserves native operation order");
    model.frame = 0u;
    runtime.frame_order.clear();
    Check(pal4::VisualModel::UpdateFrameTransform(model, runtime) == 0 &&
              pal4::VisualModel::SetRotation(
                  model, {1.0f, 2.0f, 3.0f}, false, runtime) == 0 &&
              runtime.frame_order.empty(),
          "null frame returns zero after setter stores its vector");
}

struct DestroyRuntime final : pal4::IVisualModelDestroyRuntime {
    std::uint32_t world{};
    std::vector<std::string> calls;
    std::uint32_t FindClumpWorld(std::uint32_t clump) override {
        Check(clump == 0x1234u || clump == 0u,
              "world lookup receives current clump");
        calls.emplace_back("find_world");
        return world;
    }
    void RemoveClumpFromWorld(std::uint32_t input_world,
                              std::uint32_t clump) override {
        Check(input_world == world && clump == 0x1234u,
              "world removal receives lookup result and clump");
        calls.emplace_back("remove_world");
    }
    void ClearClumpFrameLink(std::uint32_t clump) override {
        Check(clump == 0x1234u, "clump frame link cleared first");
        calls.emplace_back("clear_link");
    }
    void DestroyClump(std::uint32_t clump) override {
        Check(clump == 0x1234u, "destroy receives original clump");
        calls.emplace_back("destroy_clump");
    }
    void DestroyFrame(std::uint32_t frame) override {
        Check(frame == 0x4000u, "destroy receives original frame");
        calls.emplace_back("destroy_frame");
    }
};

void TestDestroy() {
    DestroyRuntime runtime;
    pal4::VisualModelState model;
    model.clump = 0x1234u;
    model.frame = 0x4000u;
    pal4::VisualModel::Destroy(model, runtime);
    Check(runtime.calls.empty() && model.clump == 0x1234u,
          "inactive destruction leaves model untouched");
    model.active = 1u;
    pal4::VisualModel::Destroy(model, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "find_world", "clear_link", "destroy_clump",
              "destroy_frame"} &&
              model.active == 0u && model.clump == 0u &&
              model.frame == 0u,
          "active destruction removes clump and frame in native order");
    runtime.calls.clear();
    model.active = 1u;
    model.clump = 0x1234u;
    model.frame = 0x4000u;
    runtime.world = 0x9876u;
    pal4::VisualModel::Destroy(model, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "find_world", "remove_world", "clear_link",
              "destroy_clump", "destroy_frame"},
          "world-owned clump is removed before destruction");
    runtime.calls.clear();
    runtime.world = 0u;
    model.active = 1u;
    model.clump = 0u;
    model.frame = 0u;
    pal4::VisualModel::Destroy(model, runtime);
    Check(runtime.calls == std::vector<std::string>{"find_world"} &&
              model.active == 0u,
          "active model with null resources only queries world");
}

struct RenderRuntime final : pal4::IVisualModelRenderRuntime {
    std::vector<std::string> calls;
    std::uint32_t state_20{};
    std::uint32_t GetRenderState(std::uint32_t state) override {
        calls.push_back("get:" + std::to_string(state));
        return state + 100u;
    }
    void SetRenderState(std::uint32_t state,
                        std::uint32_t value) override {
        if (state == 20u && value < 100u) state_20 = value;
        calls.push_back("set:" + std::to_string(state) + ":" +
                        std::to_string(value));
    }
    std::uint32_t GetDeviceState(std::uint32_t state) override {
        Check(state == 20u, "native device state 20 is saved");
        calls.emplace_back("device_get:20");
        return 77u;
    }
    void SetDeviceState(std::uint32_t state,
                        std::uint32_t value) override {
        calls.push_back("device_set:" + std::to_string(state) + ":" +
                        std::to_string(value));
    }
    void RenderClump(std::uint32_t clump) override {
        Check(clump == 0x1234u, "render targets model clump");
        calls.emplace_back("render_clump");
    }
    void SetTextureStage(std::uint32_t first,
                         std::uint32_t second,
                         std::uint32_t stage) override {
        Check(first == 0x1111u && second == 0x2222u,
              "texture stage receives register arguments");
        calls.push_back("texture_stage:" + std::to_string(stage));
    }
    int ClearStreamSource(std::uint32_t stream) override {
        calls.push_back("clear_stream:" + std::to_string(stream));
        return stream == 1u ? 42 : 0;
    }
};

void TestRender() {
    RenderRuntime runtime;
    pal4::VisualModelState model;
    model.clump = 0x1234u;
    Check(pal4::VisualModel::Render(
              model, 0x1111u, 0x2222u, runtime) == 0 &&
              runtime.calls.empty(),
          "inactive render does not touch D3D state");
    model.active = 1u;
    for (int property = 0; property != 2; ++property) {
        runtime.calls.clear();
        model.property = static_cast<std::uint8_t>(property);
        Check(pal4::VisualModel::Render(
                  model, 0x1111u, 0x2222u, runtime) == 42 &&
                  runtime.state_20 == (property != 0 ? 2u : 6u),
              "render state 20 follows property byte and returns final stream result");
        const std::vector<std::string> expected{
            "get:27", "get:19", "get:20", "set:27:1",
            "set:19:5", property != 0 ? "set:20:2" : "set:20:6",
            "get:7", "get:14", "set:7:1", "set:14:0",
            "device_get:20", "device_set:20:1", "render_clump",
            "device_set:20:77", "device_set:1:0",
            "set:7:107", "set:14:114", "set:27:127",
            "set:19:119", "set:20:120",
            "texture_stage:0", "texture_stage:1",
            "clear_stream:0", "clear_stream:1"};
        Check(runtime.calls == expected,
              "render saves, sets and restores D3D states in assembly order");
    }
}

void TestMaterialFlags() {
    Runtime runtime;
    pal4::VisualModelState model;
    model.clump = 0x1234u;
    Check(pal4::VisualModel::SetMaterialFlags(
              model, 0xff112233u, runtime) == 37 &&
              model.material_flags == 0xfe112233u &&
              runtime.flags == 0xfe112233u &&
              runtime.clump == model.clump,
          "0xff high byte changes to 0xfe before forwarding");
    Check(pal4::VisualModel::SetMaterialFlags(
              model, 0xfeabcdefu, runtime) == 37 &&
              model.material_flags == 0xfeabcdefu &&
              runtime.flags == 0xfeabcdefu,
          "other high bytes remain unchanged");
    Check(pal4::VisualModel::SetMaterialFlags(
              model, 0u, runtime) == 37 &&
              model.material_flags == 0u &&
              runtime.material_calls == 3 && runtime.frame_calls == 0,
          "zero flags forward without frame update");
}

}  // namespace

int main() {
    TestVectors();
    TestMaterialFlags();
    TestDestroy();
    TestRender();
}
