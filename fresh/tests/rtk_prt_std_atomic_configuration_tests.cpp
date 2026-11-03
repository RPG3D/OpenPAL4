#include "pal4/rtk_prt_std_atomic_configuration.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstring>
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

std::uint8_t OwnerByte(const pal4::RtkPrtStdAtomicObjectState& state,
                       std::uint32_t offset) {
    return static_cast<std::uint8_t>(state.words[offset / 4u] >>
                                     ((offset % 4u) * 8u));
}

struct Runtime final : pal4::IRtkPrtStdAtomicConfigurationRuntime {
    std::array<std::uint8_t, 0x1b8u> bytes{};
    std::map<std::string, std::int32_t> ints;
    std::map<std::string, float> floats;
    std::map<std::string, std::string> strings;
    std::vector<std::string> reads;
    std::vector<std::string> events;
    std::uint32_t allocation = 0x3000u;
    std::uint32_t constructed = 0x3000u;
    std::uint32_t texture_found = 0u;
    std::uint32_t texture_loaded = 0x7000u;
    std::uint32_t texture_destroyed = 0u;
    std::uint32_t frame_updates = 0u;
    bool initialize_result = true;
    bool not_point = false;
    bool axis_xz = false;
    bool face_point = false;
    bool alpha_blend = false;
    std::uint32_t emitter_type = 99u;
    std::array<float, 3> emitter_size{};
    std::string texture_path;

    std::uint32_t AllocateResource(std::uint32_t size) override {
        Check(size == 0x1b8u, "native particle resource size");
        events.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t ConstructResource(std::uint32_t memory) override {
        Check(memory == allocation, "resource constructor address");
        events.emplace_back("construct");
        return constructed;
    }
    void DestroyResource(std::uint32_t resource) override {
        Check(resource == constructed, "resource destructor address");
        events.emplace_back("destroy-resource");
    }
    void FreeResource(std::uint32_t memory) override {
        Check(memory == allocation || memory == constructed,
              "resource free address");
        events.emplace_back("free-resource");
    }
    std::int32_t ReadInt(std::uint32_t document, std::string_view key,
                         std::int32_t fallback) override {
        Check(document == 0x2000u, "XML document for int");
        reads.push_back("I:" + std::string(key));
        const auto found = ints.find(std::string(key));
        return found == ints.end() ? fallback : found->second;
    }
    float ReadFloat(std::uint32_t document, std::string_view key,
                    float fallback) override {
        Check(document == 0x2000u, "XML document for float");
        reads.push_back("F:" + std::string(key));
        const auto found = floats.find(std::string(key));
        return found == floats.end() ? fallback : found->second;
    }
    std::string ReadString(std::uint32_t document, std::string_view key,
                           std::string_view fallback) override {
        Check(document == 0x2000u, "XML document for string");
        reads.push_back("S:" + std::string(key));
        if (key == "Attr_Prt_EmitterType")
            Check(fallback == "\xB3\xA4\xB7\xBD\xCC\xE5",
                  "native GBK emitter fallback");
        const auto found = strings.find(std::string(key));
        return found == strings.end() ? std::string(fallback) : found->second;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        const auto offset = address - constructed;
        Check(address >= constructed && offset + 4u <= bytes.size(),
              "resource word offset");
        std::memcpy(bytes.data() + offset, &value, 4u);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        const auto offset = address - constructed;
        Check(address >= constructed && offset < bytes.size(),
              "resource byte offset");
        bytes[offset] = value;
    }
    void WriteFloat(std::uint32_t address, float value) override {
        WriteWord(address, std::bit_cast<std::uint32_t>(value));
    }
    void UpdateFrame(std::uint32_t resource) override {
        Check(resource == constructed, "frame update resource");
        ++frame_updates;
        events.emplace_back("frame");
    }
    std::uint32_t FindTexture(std::string_view path) override {
        texture_path = path;
        events.emplace_back("find-texture");
        return texture_found;
    }
    std::uint32_t LoadTexture(std::string_view path) override {
        Check(path == texture_path, "texture lookup/load path");
        events.emplace_back("load-texture");
        return texture_loaded;
    }
    void DestroyTexture(std::uint32_t texture) override {
        texture_destroyed = texture;
        events.emplace_back("destroy-texture");
    }
    void SetProperties(std::uint32_t resource, bool enabled) override {
        Check(resource == constructed, "particle property target");
        alpha_blend = enabled;
        events.emplace_back("properties");
    }
    bool InitializeResource(std::uint32_t resource, bool point_mode,
                            std::uint32_t texture, std::uint32_t type,
                            float x, float y, float z, bool xz) override {
        Check(resource == constructed &&
                  texture == (texture_found ? texture_found : texture_loaded),
              "particle initialization resource and texture");
        not_point = point_mode;
        emitter_type = type;
        emitter_size = {x, y, z};
        axis_xz = xz;
        events.emplace_back("initialize");
        return initialize_result;
    }
    void SetCollectionState(std::uint32_t resource, bool value) override {
        Check(resource == constructed, "collection state target");
        face_point = value;
        events.emplace_back("collection");
    }

    std::uint32_t Word(std::uint32_t offset) const {
        std::uint32_t value = 0u;
        std::memcpy(&value, bytes.data() + offset, 4u);
        return value;
    }
    float Float(std::uint32_t offset) const {
        return std::bit_cast<float>(Word(offset));
    }
};

void CheckReadOrder(const std::vector<std::string>& actual) {
    const std::vector<std::string> expected{
        "I:Attr_Prt_MaxPrt", "I:Attr_Prt_PrtEmit", "I:Attr_Prt_PrtEmit_Bias",
        "F:Attr_Prt_EmitGap", "F:Attr_Prt_EmitGap_Bias",
        "I:Attr_Prt_Alpha_AutoGradual", "F:Attr_PrtSize_x", "F:Attr_PrtSize_y",
        "F:Attr_Prt_2DRotate", "F:Attr_Prt_PrtLife",
        "F:Attr_Prt_PrtLife_Bias", "F:Attr_Prt_InitVel",
        "F:Attr_Prt_initVel_Bias", "F:Attr_Prt_InitDir_x",
        "F:Attr_Prt_InitDir_y", "F:Attr_Prt_InitDir_z",
        "F:Attr_Prt_InitDir_Bias_x", "F:Attr_Prt_InitDir_Bias_y",
        "F:Attr_Prt_InitDir_Bias_z", "I:Attr_Prt_Color_Red",
        "I:Attr_Prt_Color_Green", "I:Attr_Prt_Color_Blue",
        "I:Attr_Prt_Color_Alpha", "F:Attr_Prt_Gravitate",
        "F:Attr_Prt_Gravitate_Pos_x", "F:Attr_Prt_Gravitate_Pos_y",
        "F:Attr_Prt_Gravitate_Pos_z", "F:Attr_Prt_Force_x",
        "F:Attr_Prt_Force_y", "F:Attr_Prt_Force_z",
        "I:Attr_Prt_ColorGradual", "I:Attr_Prt_StartCol_Red",
        "I:Attr_Prt_StartCol_Green", "I:Attr_Prt_StartCol_Blue",
        "I:Attr_Prt_StartCol_Alpha", "I:Attr_Prt_StartColBias_Red",
        "I:Attr_Prt_StartColBias_Green", "I:Attr_Prt_StartColBias_Blue",
        "I:Attr_Prt_StartColBias_Alpha", "I:Attr_Prt_EndCol_Red",
        "I:Attr_Prt_EndCol_Green", "I:Attr_Prt_EndCol_Blue",
        "I:Attr_Prt_EndCol_Alpha", "I:Attr_Prt_EndColBias_Red",
        "I:Attr_Prt_EndColBias_Green", "I:Attr_Prt_EndColBias_Blue",
        "I:Attr_Prt_EndColBias_Alpha", "I:Attr_Prt_SizeGradual",
        "F:Attr_Prt_StartSize_x", "F:Attr_Prt_StartSize_y",
        "F:Attr_Prt_StartSizeBias_x", "F:Attr_Prt_StartSizeBias_y",
        "F:Attr_Prt_EndSize_x", "F:Attr_Prt_EndSize_y",
        "F:Attr_Prt_EndSizeBias_x", "F:Attr_Prt_EndSizeBias_y",
        "I:Attr_Prt_2DRotateGradual", "F:Attr_Prt_Start2DRotate",
        "F:Attr_Prt_Start2DRotateBias", "F:Attr_Prt_End2DRotate",
        "F:Attr_Prt_End2DRotateBias", "F:Attr_Prt_Atomic_Rotate_x",
        "F:Attr_Prt_Atomic_Rotate_y", "F:Attr_Prt_Atomic_Rotate_z",
        "F:Attr_Prt_Atomic_Scale_x", "F:Attr_Prt_Atomic_Scale_y",
        "F:Attr_Prt_Atomic_Scale_z", "F:Attr_Prt_Atomic_Position_x",
        "F:Attr_Prt_Atomic_Position_y", "F:Attr_Prt_Atomic_Position_z",
        "S:Attr_Prt_EmitterType", "F:Attr_Prt_Size_x",
        "F:Attr_Prt_Size_y", "F:Attr_Prt_Size_z", "S:Attr_Prt_Texture",
        "F:Attr_PreRender_Time", "I:Attr_Render_BeforeClump",
        "I:Attr_Prt_Is_Billboard", "I:Attr_Prt_Is_Point",
        "I:Attr_Prt_Axis_XZ", "I:Attr_Prt_Axis_FacePoint",
        "I:Attr_Prt_AlphaBlend_off", "I:Attr_prt_Inherit_Frame",
        "I:Attr_Prt_Strip", "I:Attr_Prt_Strip_Twice"};
    Check(actual == expected, "all XML attributes retain native read order and type");
}

void TestDefaults() {
    Runtime runtime;
    pal4::RtkPrtStdAtomicObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Check(!pal4::RtkPrtStdAtomicConfiguration::Load(state, 0u, runtime) &&
              runtime.events.empty(), "null config does not allocate");
    Check(pal4::RtkPrtStdAtomicConfiguration::Load(state, 0x2000u, runtime),
          "default particle configuration initializes");
    CheckReadOrder(runtime.reads);
    Check(state.words[23u] == 0x3000u && OwnerByte(state, 4u) == 1u &&
              OwnerByte(state, 9u) == 0u && OwnerByte(state, 11u) == 0u &&
              state.words[22u] == 0u && state.words[24u] == 0xffffffffu,
          "owner resource, activity, flags, render time and packed color");
    Check(runtime.Word(200u) == 100u && runtime.Word(260u) == 0u &&
              runtime.Word(264u) == 0u && runtime.Float(268u) == 0.0f &&
              runtime.Float(272u) == 0.0f && runtime.bytes[2u] == 1u &&
              runtime.Float(228u) == 4.0f && runtime.Float(232u) == 4.0f &&
              runtime.Float(236u) == 0.0f,
          "count, emission, particle dimensions and angle defaults");
    for (const auto offset : {212u, 216u, 220u, 224u})
        Check(runtime.Word(offset) == 0x3f800000u,
              "255 color uses the original float coefficient and produces exact1");
    for (std::uint32_t i = 0u; i != 16u; ++i)
        Check(runtime.Float(320u + 4u * i) == 0.0f,
              "gradient colors default to zero");
    Check(runtime.bytes[316u] == 1u && runtime.bytes[384u] == 1u &&
              runtime.bytes[420u] == 0u && runtime.frame_updates == 3u,
          "gradient gates and three frame rebuilds");
    for (const auto offset : {100u, 104u, 108u})
        Check(runtime.Float(offset) == 1.0f,
              "atomic scale defaults to identity");
    Check(runtime.texture_path == "gamedata\\effect\\tex\\" &&
              runtime.texture_destroyed == 0x7000u &&
              runtime.not_point && runtime.emitter_type == 0u &&
              runtime.emitter_size == std::array<float, 3>{0.0f, 0.0f, 0.0f} &&
              !runtime.axis_xz && !runtime.face_point && runtime.alpha_blend,
          "default texture, emitter and render options");
    Check(runtime.events.back() == "destroy-texture" &&
              runtime.events[runtime.events.size() - 2u] == "collection",
          "successful initialization sets collection state before texture release");
}

void TestOverridesAndFailure() {
    Runtime runtime;
    runtime.ints = {{"Attr_Prt_MaxPrt", 73}, {"Attr_Prt_PrtEmit", 9},
                    {"Attr_Prt_Color_Red", 258}, {"Attr_Prt_StartCol_Red", 128},
                    {"Attr_Prt_Is_Billboard", 1}, {"Attr_Prt_Is_Point", 1},
                    {"Attr_Prt_Axis_XZ", 1}, {"Attr_Prt_Axis_FacePoint", 1},
                    {"Attr_Prt_AlphaBlend_off", 1},
                    {"Attr_prt_Inherit_Frame", 1}, {"Attr_Prt_Strip", 1},
                    {"Attr_Prt_Strip_Twice", 0}};
    runtime.floats = {{"Attr_Prt_PrtLife", 3.5f},
                      {"Attr_Prt_Atomic_Rotate_x", 13.0f},
                      {"Attr_Prt_Atomic_Scale_y", 2.0f},
                      {"Attr_Prt_Atomic_Position_z", 7.0f},
                      {"Attr_Prt_Size_x", 10.0f},
                      {"Attr_Prt_Size_y", 11.0f},
                      {"Attr_Prt_Size_z", 12.0f}};
    runtime.strings = {{"Attr_Prt_EmitterType", "\xD4\xB2\xC7\xF2\xCC\xE5"},
                       {"Attr_Prt_Texture", "gloss_64"}};
    runtime.texture_found = 0x7200u;
    pal4::RtkPrtStdAtomicObjectState state{};
    Check(pal4::RtkPrtStdAtomicConfiguration::Load(state, 0x2000u, runtime),
          "overridden particle configuration initializes");
    Check(runtime.Word(200u) == 73u && runtime.Word(260u) == 9u &&
              runtime.Float(276u) == 3.5f && runtime.Float(88u) == 13.0f &&
              runtime.Float(104u) == 2.0f && runtime.Float(120u) == 7.0f &&
              state.words[25u] == std::bit_cast<std::uint32_t>(13.0f) &&
              state.words[29u] == std::bit_cast<std::uint32_t>(2.0f) &&
              state.words[33u] == std::bit_cast<std::uint32_t>(7.0f),
          "typed overrides reach resource and cached owner transforms");
    Check((state.words[24u] & 0xffu) == 2u &&
              runtime.Float(320u) > 0.5f && runtime.Float(320u) < 0.502f &&
              runtime.Word(124u) == 1u && runtime.bytes[4u] == 1u &&
              runtime.bytes[53u] == 1u && runtime.bytes[54u] == 0u,
          "color truncation, gradient normalization and particle flags");
    Check(runtime.texture_path == "gamedata\\effect\\tex\\gloss_64" &&
              runtime.texture_destroyed == 0x7200u &&
              runtime.emitter_type == 2u && !runtime.not_point &&
              runtime.axis_xz && runtime.face_point && !runtime.alpha_blend &&
              runtime.emitter_size == std::array<float, 3>{10.0f, 11.0f, 12.0f},
          "texture hit, sphere emitter and option forwarding");
    Check(std::find(runtime.events.begin(), runtime.events.end(),
                    "load-texture") == runtime.events.end(),
          "found texture skips fallback load");

    Runtime failed;
    failed.initialize_result = false;
    pal4::RtkPrtStdAtomicObjectState failed_state{};
    failed_state.words.fill(0xababababu);
    Check(!pal4::RtkPrtStdAtomicConfiguration::Load(
              failed_state, 0x2000u, failed) &&
              OwnerByte(failed_state, 4u) == 0u &&
              OwnerByte(failed_state, 11u) == 0u &&
              failed_state.words[23u] == 0u &&
              failed_state.words[25u] == 0xababababu &&
              failed.events[failed.events.size() - 3u] == "destroy-resource" &&
              failed.events[failed.events.size() - 2u] == "free-resource" &&
              failed.events.back() == "destroy-texture",
          "failed initialization destroys resources and preserves uncached transforms");
}

void TestAllocationFailure() {
    Runtime runtime;
    runtime.allocation = 0u;
    pal4::RtkPrtStdAtomicObjectState state{};
    Check(!pal4::RtkPrtStdAtomicConfiguration::Load(state, 0x2000u, runtime) &&
              state.words[23u] == 0u && runtime.reads.empty(),
          "allocation failure stops before attribute reads");
    runtime.allocation = 0x3000u;
    runtime.constructed = 0u;
    Check(!pal4::RtkPrtStdAtomicConfiguration::Load(state, 0x2000u, runtime) &&
              state.words[23u] == 0u && runtime.events.back() == "free-resource",
          "constructor failure frees allocated resource");
}

}  // namespace

int main() {
    TestDefaults();
    TestOverridesAndFailure();
    TestAllocationFailure();
}
