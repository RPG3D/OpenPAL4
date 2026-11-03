#include "pal4/cylinder_object_config.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICylinderObjectConfigRuntime {
    std::vector<std::string> calls;
    std::map<std::string, std::int32_t> integers;
    std::map<std::string, float> floats;
    std::string texture_name = "flame.dds";
    std::uint32_t found_texture = 0u;
    std::uint32_t model_address = 0x2000u;
    bool create_succeeds = true;
    bool throw_on_find = false;
    pal4::CylinderVisualParameters parameters{};
    std::array<float, 3> position{};
    std::array<float, 3> rotation{};
    std::array<float, 3> scale{};
    std::uint32_t color = 0u;

    std::string_view ReadTextureName(std::uint32_t document,
                                     std::string_view key,
                                     std::uint32_t default_address) override {
        Check(document == 0x1000u && key == "Attr_Cylinder_Texture" &&
                  default_address == 0x8e425cu,
              "texture XML key and native default address");
        calls.push_back("texture_name");
        return texture_name;
    }
    std::int32_t ReadInt(std::uint32_t document, std::string_view key,
                         std::int32_t fallback) override {
        Check(document == 0x1000u, "integer XML document");
        calls.push_back("int:" + std::string(key));
        const auto it = integers.find(std::string(key));
        return it == integers.end() ? fallback : it->second;
    }
    float ReadFloat(std::uint32_t document, std::string_view key,
                    float fallback) override {
        Check(document == 0x1000u, "float XML document");
        calls.push_back("float:" + std::string(key));
        const auto it = floats.find(std::string(key));
        return it == floats.end() ? fallback : it->second;
    }
    std::uint32_t FindTexture(std::string_view path) override {
        Check(path == "gamedata\\effect\\tex\\flame.dds",
              "prefixed texture path");
        calls.push_back("find_texture");
        if (throw_on_find)
            throw std::runtime_error("texture");
        return found_texture;
    }
    std::uint32_t LoadTexture(std::string_view path) override {
        Check(path == "gamedata\\effect\\tex\\flame.dds",
              "fallback texture load path");
        calls.push_back("load_texture");
        return 0x3000u;
    }
    std::uint32_t AllocateModel(std::uint32_t bytes) override {
        Check(bytes == 52u, "visual model allocation size");
        calls.push_back("allocate_model");
        return model_address;
    }
    void InitializeModel(std::uint32_t model) override {
        Check(model == 0x2000u, "visual model initialization target");
        calls.push_back("initialize_model");
    }
    bool CreateVisualModel(
        std::uint32_t model, std::uint32_t texture,
        const pal4::CylinderVisualParameters& value) override {
        Check(model == model_address &&
                  texture == (found_texture == 0u ? 0x3000u : found_texture),
              "visual model and selected texture");
        parameters = value;
        calls.push_back("create_model");
        return create_succeeds;
    }
    void SetPosition(std::uint32_t model,
                     const std::array<float, 3>& value,
                     std::uint32_t mode) override {
        Check(model == 0x2000u && mode == 0u, "position callback");
        position = value;
        calls.push_back("position");
    }
    void SetRotation(std::uint32_t model,
                     const std::array<float, 3>& value,
                     std::uint32_t mode) override {
        Check(model == 0x2000u && mode == 0u, "rotation callback");
        rotation = value;
        calls.push_back("rotation");
    }
    void SetScale(std::uint32_t model,
                  const std::array<float, 3>& value,
                  std::uint32_t mode) override {
        Check(model == 0x2000u && mode == 0u, "scale callback");
        scale = value;
        calls.push_back("scale");
    }
    void SetColor(std::uint32_t model, std::uint32_t value) override {
        Check(model == 0x2000u, "color callback target");
        color = value;
        calls.push_back("color");
    }
    std::uint32_t CopyDocument(std::uint32_t document) override {
        Check(document == 0x1000u, "source document clone");
        calls.push_back("copy_document");
        return 0x4000u;
    }
    void DestroyModel(std::uint32_t model) override {
        Check(model == 0x2000u, "failed model destruction");
        calls.push_back("destroy_model");
    }
    void FreeModel(std::uint32_t model) override {
        Check(model == 0x2000u, "failed model free");
        calls.push_back("free_model");
    }
    void ReleaseTexturePath(std::string_view path) override {
        Check(path == "gamedata\\effect\\tex\\flame.dds",
              "temporary path release");
        calls.push_back("release_path");
    }
};

bool Contains(const std::vector<std::string>& calls,
              const std::string_view value) {
    for (const auto& call : calls)
        if (call == value)
            return true;
    return false;
}

void TestDefaultSuccessAndGate() {
    pal4::CylinderObjectState state{};
    Runtime runtime;
    Check(pal4::CylinderObjectConfiguration::Load(
              state, 0u, runtime) == 0u && runtime.calls.empty(),
          "null XML document has no side effects");
    Check(pal4::CylinderObjectConfiguration::Load(
              state, 0x1000u, runtime) == 1u &&
              state.words[22u] == 0x2000u &&
              state.words[23u] == 0x4000u &&
              (state.words[1u] & 0xffu) == 1u &&
              static_cast<std::uint8_t>(state.words[2u] >> 16u) == 1u &&
              static_cast<std::uint8_t>(state.words[2u] >> 24u) == 0u &&
              state.words[25u] == 0xffffffffu,
          "successful load caches model, XML, flags and default color");
    Check(runtime.parameters.border_count == 5 &&
              runtime.parameters.height == 5.0f &&
              runtime.parameters.bottom_radius == 6.0f &&
              runtime.parameters.top_radius == 3.0f &&
              runtime.parameters.rasterize &&
              runtime.parameters.shrink_size == 2.0f &&
              runtime.parameters.angle == 360 &&
              !runtime.parameters.flip_uv &&
              runtime.parameters.center &&
              runtime.parameters.auto_alpha_blend,
          "native XML lookup defaults reach visual constructor");
    Check(runtime.position == std::array<float, 3>{0.0f, 0.0f, 0.0f} &&
              runtime.rotation == std::array<float, 3>{0.0f, 0.0f, 0.0f} &&
              runtime.scale == std::array<float, 3>{1.0f, 1.0f, 1.0f} &&
              runtime.color == 0xffffffffu &&
              state.words[26u] == std::bit_cast<std::uint32_t>(0.0f) &&
              state.words[29u] == std::bit_cast<std::uint32_t>(1.0f) &&
              state.words[32u] == std::bit_cast<std::uint32_t>(0.0f),
          "initial transforms and cached transform field offsets");
    Check(Contains(runtime.calls, "find_texture") &&
              Contains(runtime.calls, "load_texture") &&
              Contains(runtime.calls, "initialize_model") &&
              Contains(runtime.calls, "copy_document") &&
              runtime.calls.back() == "release_path",
          "texture fallback, model initialization and cleanup execute");
}

void TestOverridesAndFailure() {
    pal4::CylinderObjectState state{};
    Runtime runtime;
    runtime.found_texture = 0x5555u;
    runtime.integers = {
        {"Attr_Auto_AlphaBlend", 0},
        {"Attr_Cylinder_BorderNumb", 12},
        {"Attr_Cylinder_Angel", 180},
        {"Attr_Cylinder_Asterialize", -1},
        {"Attr_Cylinder_Vertex_Col_Red", 258},
        {"Attr_Cylinder_Vertex_Col_Green", 3},
        {"Attr_Cylinder_Vertex_Col_Blue", 4},
        {"Attr_Cylinder_Vertex_Col_Alpha", 5},
        {"Attr_Cylinder_FlipUV", 2},
        {"Attr_Cylinder_Center", 0}};
    runtime.floats = {
        {"Attr_Cylinder_Height", 8.0f},
        {"Attr_Cylinder_Bottom_Raidus", 9.0f},
        {"Attr_Cylinder_Top_Raidus", 10.0f},
        {"Attr_Cylinder_Shrink_Size", 0.5f},
        {"Attr_Cylinder_Rotate_x", 11.0f},
        {"Attr_Cylinder_Scale_y", 12.0f},
        {"Attr_Cylinder_Position_z", 13.0f}};
    Check(pal4::CylinderObjectConfiguration::Load(
              state, 0x1000u, runtime) == 1u &&
              !Contains(runtime.calls, "load_texture") &&
              runtime.parameters.border_count == 12 &&
              runtime.parameters.height == 8.0f &&
              runtime.parameters.bottom_radius == 9.0f &&
              runtime.parameters.top_radius == 10.0f &&
              runtime.parameters.shrink_size == 0.5f &&
              runtime.parameters.angle == 180 &&
              runtime.parameters.flip_uv &&
              !runtime.parameters.center &&
              !runtime.parameters.rasterize &&
              !runtime.parameters.auto_alpha_blend &&
              runtime.color == 0x05040302u &&
              runtime.rotation[0u] == 11.0f &&
              runtime.scale[1u] == 12.0f &&
              runtime.position[2u] == 13.0f,
          "XML overrides, signed bools and byte-truncated color are applied");

    state = {};
    runtime = Runtime{};
    runtime.create_succeeds = false;
    Check(pal4::CylinderObjectConfiguration::Load(
              state, 0x1000u, runtime) == 0u &&
              state.words[22u] == 0u && state.words[23u] == 0u &&
              state.words[25u] == 0xffffffffu &&
              Contains(runtime.calls, "destroy_model") &&
              Contains(runtime.calls, "free_model") &&
              !Contains(runtime.calls, "copy_document") &&
              runtime.calls.back() == "release_path",
          "failed visual creation destroys resource but caches parsed values");
    state = {};
    runtime = Runtime{};
    runtime.throw_on_find = true;
    try {
        static_cast<void>(pal4::CylinderObjectConfiguration::Load(
            state, 0x1000u, runtime));
        Check(false, "texture lookup must throw");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "release_path",
              "temporary texture path releases on exception");
    }
}

void TestReset() {
    pal4::CylinderObjectState state{};
    Runtime runtime;
    state.words[22u] = 0x2000u;
    state.words[25u] = 0x12345678u;
    state.words[26u] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[27u] = std::bit_cast<std::uint32_t>(2.0f);
    state.words[28u] = std::bit_cast<std::uint32_t>(3.0f);
    state.words[29u] = std::bit_cast<std::uint32_t>(4.0f);
    state.words[30u] = std::bit_cast<std::uint32_t>(5.0f);
    state.words[31u] = std::bit_cast<std::uint32_t>(6.0f);
    state.words[32u] = std::bit_cast<std::uint32_t>(7.0f);
    state.words[33u] = std::bit_cast<std::uint32_t>(8.0f);
    state.words[34u] = std::bit_cast<std::uint32_t>(9.0f);
    state.words[2u] = 0x55abcdefu;
    Check(pal4::CylinderObjectConfiguration::Reset(state, runtime) == 0u &&
              runtime.calls.empty() && state.words[2u] == 0x55abcdefu,
          "inactive reset leaves cached fields and mode unchanged");
    state.words[1u] = 1u;
    Check(pal4::CylinderObjectConfiguration::Reset(state, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "color", "rotation", "scale", "position"} &&
              runtime.color == 0x12345678u &&
              runtime.rotation == std::array<float, 3>{1.0f, 2.0f, 3.0f} &&
              runtime.scale == std::array<float, 3>{4.0f, 5.0f, 6.0f} &&
              runtime.position == std::array<float, 3>{7.0f, 8.0f, 9.0f} &&
              state.words[2u] == 0x00abcdefu,
          "active reset reapplies cached values in native order");
}

}  // namespace

int main() {
    TestDefaultSuccessAndGate();
    TestOverridesAndFailure();
    TestReset();
}
