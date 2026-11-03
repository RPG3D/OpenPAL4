#include "pal4/sequence_anim_configuration.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
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

float FloatWord(const pal4::SequenceAnimObjectState& state,
                std::uint32_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}
std::uint8_t Byte(const pal4::SequenceAnimObjectState& state,
                  std::uint32_t offset) {
    return static_cast<std::uint8_t>(state.words[offset / 4u] >>
                                     ((offset % 4u) * 8u));
}

struct Runtime final : pal4::ISequenceAnimConfigurationRuntime {
    std::map<std::string, std::int32_t> ints;
    std::map<std::string, float> floats;
    std::map<std::string, std::string> strings;
    std::vector<std::string> reads;
    std::vector<std::string> events;
    std::array<std::uint8_t, 0x34u> resource_bytes{};
    std::uint32_t texture_found = 0x7000u;
    std::uint32_t texture_loaded = 0x7100u;
    std::uint32_t allocation = 0x3000u;
    std::uint32_t color = 0u;
    std::array<std::uint32_t, 4> corners{};
    bool initialize_result = true;
    float expected_width = 32.0f;

    std::int32_t ReadInt(std::uint32_t document, std::string_view key,
                         std::int32_t fallback) override {
        Check(document == 0x2000u, "integer document address");
        reads.push_back("I:" + std::string(key));
        const auto found = ints.find(std::string(key));
        return found == ints.end() ? fallback : found->second;
    }
    float ReadFloat(std::uint32_t document, std::string_view key,
                    float fallback) override {
        Check(document == 0x2000u, "float document address");
        reads.push_back("F:" + std::string(key));
        const auto found = floats.find(std::string(key));
        return found == floats.end() ? fallback : found->second;
    }
    std::string ReadString(std::uint32_t document, std::string_view key,
                           std::string_view fallback) override {
        Check(document == 0x2000u && key == "Attr_SequenceAnim_Texture" &&
                  fallback == "\xCE\xB4\xC9\xE8\xD6\xC3",
              "native GBK missing-texture fallback");
        reads.push_back("S:" + std::string(key));
        const auto found = strings.find(std::string(key));
        return found == strings.end() ? std::string(fallback) : found->second;
    }
    std::uint32_t FindTexture(std::string_view path) override {
        Check(path == "gamedata\\effect\\tex\\smoke.png",
              "native texture root and filename");
        events.emplace_back("find");
        return texture_found;
    }
    std::uint32_t LoadTexture(std::string_view path) override {
        Check(path == "gamedata\\effect\\tex\\smoke.png",
              "texture load path");
        events.emplace_back("load");
        return texture_loaded;
    }
    std::int32_t TextureWidth(std::uint32_t texture) override {
        Check(texture == 0x7000u || texture == 0x7100u,
              "texture width handle");
        events.emplace_back("width");
        return 128;
    }
    std::int32_t TextureHeight(std::uint32_t texture) override {
        Check(texture == 0x7000u || texture == 0x7100u,
              "texture height handle");
        events.emplace_back("height");
        return 64;
    }
    std::int32_t Ftol(float value) override {
        events.emplace_back("ftol");
        return static_cast<std::int32_t>(value);
    }
    std::uint32_t AllocateResource(std::uint32_t bytes) override {
        Check(bytes == 0x34u, "52-byte native resource allocation");
        events.emplace_back("allocate");
        return allocation;
    }
    void WriteResourceWord(std::uint32_t address,
                           std::uint32_t value) override {
        const auto offset = address - allocation;
        Check(allocation != 0u && address >= allocation &&
                  offset + 4u <= resource_bytes.size(),
              "resource word write bounds");
        std::memcpy(resource_bytes.data() + offset, &value, 4u);
        events.emplace_back("word:" + std::to_string(offset));
    }
    void WriteResourceByte(std::uint32_t address,
                           std::uint8_t value) override {
        const auto offset = address - allocation;
        Check(allocation != 0u && address >= allocation &&
                  offset < resource_bytes.size(),
              "resource byte write bounds");
        resource_bytes[offset] = value;
        events.emplace_back("byte:" + std::to_string(offset));
    }
    bool InitializeResource(std::uint32_t resource, std::uint32_t texture,
                            float width, float height, bool auto_blend,
                            bool axis_billboard, bool center) override {
        const auto expected_texture = texture_found != 0u
                                          ? texture_found : texture_loaded;
        Check(resource == allocation && texture == expected_texture &&
                  width == expected_width && height == 16.0f &&
                  auto_blend && axis_billboard && !center,
              "resource init native arguments and flags");
        events.emplace_back("initialize");
        return initialize_result;
    }
    void InitializeTransform(std::uint32_t resource) override {
        Check(resource == allocation, "transform resource");
        events.emplace_back("transform");
    }
    std::uint32_t Geometry(std::uint32_t resource) override {
        Check(resource == allocation, "geometry resource");
        events.emplace_back("geometry");
        return 0x5000u;
    }
    void SetColor(std::uint32_t geometry, std::uint32_t rgba) override {
        Check(geometry == 0x5000u, "color geometry");
        color = rgba;
        events.emplace_back("color");
    }
    void UpdateGeometry(std::uint32_t geometry,
                        const std::array<std::uint32_t, 4>& values) override {
        Check(geometry == 0x5000u, "update geometry");
        corners = values;
        events.emplace_back("geometry-update");
    }
    void DeinitializeResource(std::uint32_t resource) override {
        Check(resource == allocation, "resource deinitialize address");
        events.emplace_back("deinitialize");
    }
    void FreeResource(std::uint32_t resource) override {
        Check(resource == allocation, "resource free address");
        events.emplace_back("free");
    }
    std::uint32_t Word(std::uint32_t offset) const {
        std::uint32_t value = 0u;
        std::memcpy(&value, resource_bytes.data() + offset, 4u);
        return value;
    }
};

void Configure(Runtime& runtime) {
    runtime.strings["Attr_SequenceAnim_Texture"] = "smoke.png";
    runtime.ints["Attr_SequenceAnim_FPS"] = 30;
    runtime.ints["Attr_SequenceAnim_AxisBillbord"] = 1;
    runtime.ints["Attr_Center"] = 0;
    runtime.ints["Attr_SequenceAnim_Color_Red"] = 0x123;
    runtime.ints["Attr_SequenceAnim_Color_Green"] = 0x45;
    runtime.ints["Attr_SequenceAnim_Color_Blue"] = 0x67;
    runtime.ints["Attr_SequenceAnim_Color_Alpha"] = 0x89;
    runtime.floats["Attr_SequenceAnim_SubTexWidth"] = 32.0f;
    runtime.floats["Attr_SequenceAnim_SubTexHeight"] = 16.0f;
    runtime.floats["Attr_SequenceAnim_Rotate_x"] = 15.0f;
    runtime.floats["Attr_SequenceAnim_Scale_SubTexWidth"] = 2.0f;
    runtime.floats["Attr_SequenceAnim_Scale_SubTexHeight"] = 3.0f;
    runtime.floats["Attr_SequenceAnim_Position_X"] = 4.0f;
    runtime.floats["Attr_SequenceAnim_Position_Y"] = 5.0f;
    runtime.floats["Attr_SequenceAnim_Position_Z"] = 6.0f;
}

void TestSuccess() {
    Runtime runtime;
    Configure(runtime);
    pal4::SequenceAnimObjectState state{};
    state.words[2u] = 0xabcd1234u;
    state.words[29u] = 0x55555555u;
    Check(pal4::SequenceAnimConfiguration::Load(state, 0x2000u, runtime),
          "successful configuration result");
    const std::vector<std::string> expected_reads{
        "I:Attr_SequenceAnim_FPS", "S:Attr_SequenceAnim_Texture",
        "I:Attr_Auto_AlphaBlend", "I:Attr_SequenceAnim_AxisBillbord",
        "I:Attr_Render_Last", "F:Attr_SequenceAnim_SubTexWidth",
        "F:Attr_SequenceAnim_SubTexHeight", "F:Attr_SequenceAnim_Rotate_x",
        "F:Attr_SequenceAnim_Scale_SubTexWidth",
        "F:Attr_SequenceAnim_Scale_SubTexHeight",
        "F:Attr_SequenceAnim_Position_X", "F:Attr_SequenceAnim_Position_Y",
        "F:Attr_SequenceAnim_Position_Z", "I:Attr_SequenceAnim_Color_Red",
        "I:Attr_SequenceAnim_Color_Green", "I:Attr_SequenceAnim_Color_Blue",
        "I:Attr_SequenceAnim_Color_Alpha", "I:Attr_Center"};
    Check(runtime.reads == expected_reads, "all 18 XML reads in native order");
    Check(Byte(state, 4u) == 1u && Byte(state, 8u) == 1u &&
              Byte(state, 10u) == 1u && Byte(state, 11u) == 0u,
          "activity and mode flags");
    Check(state.words[30u] == 0x3000u && state.words[35u] == 0x89674523u,
          "resource and truncated packed color");
    Check(FloatWord(state, 52u) == 1.0f / 30.0f &&
              state.words[14u] == 4u && state.words[15u] == 4u &&
              state.words[17u] == 3u,
          "frame interval and texture grid counts");
    Check(FloatWord(state, 72u) == 0.25f &&
              FloatWord(state, 76u) == 0.25f &&
              FloatWord(state, 84u) == 0.0f &&
              FloatWord(state, 88u) == 0.75f &&
              FloatWord(state, 92u) == 0.25f &&
              FloatWord(state, 96u) == 1.0f,
          "native UV step and corner fields");
    Check(FloatWord(state, 100u) == 128.0f &&
              FloatWord(state, 104u) == 64.0f &&
              FloatWord(state, 108u) == 32.0f &&
              FloatWord(state, 112u) == 16.0f,
          "texture and subtexture dimensions");
    Check(runtime.Word(24u) == std::bit_cast<std::uint32_t>(15.0f) &&
              runtime.Word(28u) == std::bit_cast<std::uint32_t>(4.0f) &&
              runtime.Word(32u) == std::bit_cast<std::uint32_t>(5.0f) &&
              runtime.Word(36u) == std::bit_cast<std::uint32_t>(6.0f) &&
              runtime.Word(40u) == std::bit_cast<std::uint32_t>(2.0f) &&
              runtime.Word(44u) == std::bit_cast<std::uint32_t>(3.0f),
          "resource transform fields");
    Check(runtime.color == 0x89674523u &&
              runtime.corners == std::array<std::uint32_t, 4>{
                  state.words[21u], state.words[22u],
                  state.words[23u], state.words[24u]},
          "geometry callbacks");
    for (std::uint32_t i = 0u; i != 4u; ++i)
        Check(state.words[31u + i] == state.words[21u + i],
              "cached geometry corner");
    Check(FloatWord(state, 144u) == 15.0f &&
              FloatWord(state, 148u) == 2.0f &&
              FloatWord(state, 152u) == 3.0f &&
              FloatWord(state, 156u) == 4.0f &&
              FloatWord(state, 160u) == 5.0f &&
              FloatWord(state, 164u) == 6.0f,
          "cached transform fields");
    Check(runtime.events.back() == "geometry-update" &&
              runtime.events[runtime.events.size() - 4u] == "transform",
          "success callback order");
}

void TestTextureAndFailure() {
    Runtime fallback;
    Configure(fallback);
    fallback.texture_found = 0u;
    fallback.texture_loaded = 0x7000u;
    fallback.initialize_result = false;
    pal4::SequenceAnimObjectState state{};
    state.words[2u] = 0xabcd1234u;
    Check(!pal4::SequenceAnimConfiguration::Load(state, 0x2000u, fallback) &&
              state.words[30u] == 0u && Byte(state, 4u) == 0u,
          "failed initialization clears native resource");
    Check(fallback.events[0u] == "find" &&
              fallback.events[1u] == "load" &&
              fallback.events[fallback.events.size() - 2u] == "deinitialize" &&
              fallback.events.back() == "free",
          "texture fallback and cleanup order");

    Runtime missing;
    Configure(missing);
    missing.texture_found = 0u;
    missing.texture_loaded = 0u;
    pal4::SequenceAnimObjectState missing_state{};
    missing_state.words[2u] = 0xabcd1234u;
    missing_state.words[25u] = 0xffffffffu;
    missing_state.words[26u] = 0xffffffffu;
    missing.initialize_result = false;
    Check(!pal4::SequenceAnimConfiguration::Load(
              missing_state, 0u, missing) && missing.events.empty(),
          "null document short-circuit");
    Check(!pal4::SequenceAnimConfiguration::Load(
              missing_state, 0x2000u, missing) &&
              FloatWord(missing_state, 52u) == -1.0f &&
              missing_state.words[25u] == 0u &&
              missing_state.words[26u] == 0u &&
              missing_state.words[30u] == 0u,
          "missing texture sentinel and resource cleanup");
}

}  // namespace

void TestRectangularGrid() {
    Runtime runtime;
    Configure(runtime);
    runtime.floats["Attr_SequenceAnim_SubTexWidth"] = 16.0f;
    runtime.expected_width = 16.0f;
    pal4::SequenceAnimObjectState state{};
    Check(pal4::SequenceAnimConfiguration::Load(state, 0x2000u, runtime),
          "rectangular atlas loads");
    Check(state.words[14u] == 8u && state.words[15u] == 4u &&
              FloatWord(state, 92u) == 0.125f &&
              FloatWord(state, 88u) == 0.75f && FloatWord(state, 96u) == 1.0f &&
              state.words[34u] == state.words[24u],
          "IDA615A80 initial and cached V extent uses row height on a rectangular grid");
}

int main() {
    TestSuccess();
    TestTextureAndFailure();
    TestRectangularGrid();
}
