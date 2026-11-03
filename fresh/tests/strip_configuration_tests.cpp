#include "pal4/strip_configuration.h"

#include <bit>
#include <cmath>
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

std::uint8_t Byte(const pal4::StripObjectState& state, std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

float Float(const pal4::StripObjectState& state, std::uint32_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}

struct Runtime final : pal4::IStripConfigurationRuntime {
    std::map<std::string, std::int32_t> ints;
    std::map<std::string, float> floats;
    std::vector<std::string> reads;
    std::vector<std::string> events;
    pal4::StripVisualParameters parameters{};
    std::uint32_t texture = 0x2000u;
    bool fail_create = false;
    std::string path;

    std::string_view ReadTextureName(
        std::uint32_t document, std::string_view key,
        std::uint32_t default_address) override {
        Check(document == 0x1000u && key == "Attr_Strip_Texture" &&
                  default_address == 0x8e425cu,
              "texture attribute and native default pointer");
        reads.emplace_back(key);
        return "trail.dds";
    }
    std::int32_t ReadInt(std::uint32_t document, std::string_view key,
                         std::int32_t default_value) override {
        Check(document == 0x1000u, "integer document");
        reads.emplace_back(key);
        const auto found = ints.find(std::string(key));
        return found == ints.end() ? default_value : found->second;
    }
    float ReadFloat(std::uint32_t document, std::string_view key,
                    float default_value) override {
        Check(document == 0x1000u, "float document");
        reads.emplace_back(key);
        const auto found = floats.find(std::string(key));
        return found == floats.end() ? default_value : found->second;
    }
    std::uint32_t FindTexture(std::string_view value) override {
        Check(value == "gamedata\\effect\\tex\\trail.dds",
              "prefixed strip texture path");
        path = value;
        events.emplace_back("find");
        return texture;
    }
    std::uint32_t LoadTexture(std::string_view value) override {
        Check(value == path, "texture fallback path");
        events.emplace_back("load");
        return texture;
    }
    std::int32_t FloatToInt(float value) override {
        events.emplace_back("ftol");
        return static_cast<std::int32_t>(value);
    }
    void TextureSize(std::uint32_t handle, std::int32_t& width,
                     std::int32_t& height) override {
        Check(handle == texture, "sequence texture handle");
        events.emplace_back("size");
        width = 64;
        height = 32;
    }
    void CreateCylinder(pal4::StripObjectState& state,
                        std::uint32_t handle,
                        const pal4::StripVisualParameters& values) override {
        Check(handle == texture && Byte(state, 140u) == 0u,
              "cylinder branch and texture");
        parameters = values;
        events.emplace_back("cylinder");
        state.words[33u] = 0x3000u;
        state.words[34u] = 0x3100u;
        if (fail_create) throw std::runtime_error("cylinder");
    }
    void CreateStraight(pal4::StripObjectState& state,
                        std::uint32_t handle,
                        const pal4::StripVisualParameters& values) override {
        Check(handle == texture && Byte(state, 140u) == 1u,
              "straight branch and texture");
        parameters = values;
        events.emplace_back("straight");
        state.words[38u] = 0x3200u;
    }
    std::uint32_t CopyDocument(std::uint32_t document) override {
        Check(document == 0x1000u, "copy source document");
        events.emplace_back("copy");
        return 0x3300u;
    }
    void ReleaseTexture(std::uint32_t handle) override {
        Check(handle == texture, "release acquired texture");
        events.emplace_back("release-texture");
    }
    void ReleaseTexturePath(std::string_view value) override {
        Check(value == "gamedata\\effect\\tex\\trail.dds",
              "release prefixed texture path");
        events.emplace_back("release-path");
    }
};

void TestReadOrder(const Runtime& runtime) {
    const std::vector<std::string> expected{
        "Attr_Strip_Texture", "Attr_Strip_Wdith", "Attr_Strip_Height",
        "Attr_Strip_FlipUV", "Attr_Auto_AlphaBlend",
        "Attr_Strip_Position_x", "Attr_Strip_Position_y",
        "Attr_Strip_Position_z", "Attr_Strip_Scale_x",
        "Attr_Strip_Scale_y", "Attr_Strip_Scale_z",
        "Attr_Strip_Rotate_x", "Attr_Strip_Rotate_y",
        "Attr_Strip_Rotate_z", "Attr_Strip_Vertex_Col_Red",
        "Attr_Strip_Vertex_Col_Green", "Attr_Strip_Vertex_Col_Blue",
        "Attr_Strip_Vertex_Col_Alpha", "Attr_Strip_Cylinder_Creator",
        "Attr_Strip_Cylinder_Border_Numb", "Attr_Strip_Cylinder_TRadius",
        "Attr_Strip_Cylinder_BRadius", "Attr_Strip_Cylinder_Loop_Numb",
        "Attr_Strip_Straight_Creator", "Attr_Strip_Straight_Face_Numb",
        "Attr_Strip_Straight_Sequence", "Attr_Strip_Straight_Seq_Width",
        "Attr_Strip_Straight_Seq_Height", "Attr_Strip_Straight_Seq_Fps",
        "Attr_Center", "Attr_Render_Last", "Attr_Strip_RS",
        "Attr_Strip_Cullmode"};
    Check(runtime.reads == expected, "native strip attribute read order");
}

void TestCylinder() {
    pal4::StripObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    runtime.floats["Attr_Strip_Wdith"] = 20.0f;
    runtime.floats["Attr_Strip_Height"] = 10.0f;
    runtime.floats["Attr_Strip_Position_x"] = 3.0f;
    runtime.floats["Attr_Strip_Scale_y"] = 2.0f;
    runtime.floats["Attr_Strip_Rotate_z"] = 45.0f;
    runtime.ints["Attr_Strip_Vertex_Col_Red"] = 300;
    runtime.ints["Attr_Strip_Vertex_Col_Alpha"] = 255;
    runtime.ints["Attr_Strip_RS"] = 1;
    Check(pal4::StripConfiguration::Load(state, 0x1000u, runtime) == 1u,
          "cylinder configuration succeeds");
    TestReadOrder(runtime);
    Check(runtime.events == std::vector<std::string>{
              "find", "cylinder", "release-texture", "copy", "release-path"},
          "cylinder native operation order");
    Check(runtime.parameters.width == 20.0f &&
              runtime.parameters.height == 10.0f &&
              runtime.parameters.cylinder_top_radius == 5.0f &&
              runtime.parameters.cylinder_bottom_radius == 10.0f &&
              runtime.parameters.cylinder_borders == 10 &&
              runtime.parameters.cylinder_loops == 1 &&
              runtime.parameters.center && runtime.parameters.render_last,
          "geometry parameters and native fallback defaults");
    Check(Byte(state, 4u) == 1u && Byte(state, 8u) == 1u &&
              Byte(state, 10u) == 1u && Byte(state, 11u) == 0u &&
              Byte(state, 60u) == 1u && Byte(state, 103u) == 0xfeu &&
              Byte(state, 128u) == 0u && Byte(state, 160u) == 0u &&
              state.words[39u] == 0x3300u,
          "flags, alpha clamp, document and state bytes");
    Check(Float(state, 88u) == 3.0f && Float(state, 68u) == 2.0f &&
              Float(state, 84u) == 45.0f &&
              state.words[57u] == state.words[25u] &&
              state.words[61u] == state.words[19u] &&
              state.words[58u] == state.words[16u] &&
              state.words[64u] == state.words[22u],
          "live transform and cached state");
}

void TestSequenceAndFallback() {
    pal4::StripObjectState state{};
    Runtime runtime;
    runtime.texture = 0u;
    runtime.ints["Attr_Strip_Cylinder_Creator"] = 0;
    runtime.ints["Attr_Strip_Straight_Creator"] = 1;
    runtime.floats["Attr_Strip_Straight_Sequence"] = 1.0f;
    Check(pal4::StripConfiguration::Load(state, 0x1000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "find", "load", "release-path"} &&
              Float(state, 220u) == 16.0f && Float(state, 224u) == 16.0f &&
              Byte(state, 4u) == 0u,
          "missing texture returns after dimension capture");

    state = pal4::StripObjectState{};
    runtime = Runtime{};
    runtime.ints["Attr_Strip_Cylinder_Creator"] = 0;
    runtime.ints["Attr_Strip_Straight_Creator"] = 1;
    runtime.floats["Attr_Strip_Straight_Sequence"] = 1.0f;
    Check(pal4::StripConfiguration::Load(state, 0x1000u, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "find", "size", "ftol", "ftol", "straight",
                  "release-texture", "copy", "release-path"},
          "straight sequence native operation order");
    Check(Byte(state, 140u) == 1u && Byte(state, 160u) == 1u &&
              state.words[42u] == 4u && state.words[43u] == 2u &&
              state.words[44u] == 0u && state.words[45u] == 1u &&
              Float(state, 164u) == 1.0f / 30.0f &&
              Float(state, 184u) == 0.25f &&
              Float(state, 188u) == 0.5f &&
              Float(state, 196u) == 0.0f &&
              Float(state, 200u) == 0.5f &&
              Float(state, 204u) == 0.25f &&
              Float(state, 208u) == 1.0f &&
              state.words[67u] == state.words[49u],
          "straight atlas counters, UVs and cached vertices");
}

void TestGuardAndException() {
    pal4::StripObjectState state{};
    Runtime runtime;
    Check(pal4::StripConfiguration::Load(state, 0u, runtime) == 0u &&
              runtime.reads.empty() && runtime.events.empty(),
          "null document does no work");
    runtime.fail_create = true;
    bool threw = false;
    try {
        (void)pal4::StripConfiguration::Load(state, 0x1000u, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events == std::vector<std::string>{
              "find", "cylinder", "release-texture", "release-path"},
          "geometry exception releases texture and path");
}

}  // namespace

int main() {
    TestCylinder();
    TestSequenceAndFallback();
    TestGuardAndException();
}
