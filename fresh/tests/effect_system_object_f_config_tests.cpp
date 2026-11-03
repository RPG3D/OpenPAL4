#include "pal4/effect_system_object_f_config.h"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
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

struct Runtime final : pal4::IEffectSystemObjectFConfigRuntime {
    std::vector<std::string> reads;
    std::vector<std::string> events;
    std::unordered_map<std::string, float> floats;
    std::unordered_map<std::string, std::int32_t> ints;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::string texture_name;
    std::string found_path;
    std::string loaded_path;
    std::uint32_t found_texture = 0x8000u;
    std::uint32_t loaded_texture = 0x8100u;
    std::uint32_t object = 0x6000u;
    std::uint32_t destroyed_texture = 0u;
    std::uint32_t color_slot = 0u;
    std::uint32_t color = 0u;
    std::array<float, 2> size{};
    std::array<std::uint8_t, 3> flags{};
    bool initialized = true;
    bool throw_initialize = false;

    std::string ReadString(std::uint32_t document, std::string_view key,
                           std::string_view fallback) override {
        Check(document == 0x5000u, "string document");
        reads.emplace_back(key);
        Check(key == "Attr_BillBord_Texture" &&
                  fallback == std::string_view("\xCE\xB4\xC9\xE8\xD6\xC3", 6),
              "native texture key and GBK fallback");
        return texture_name.empty() ? std::string(fallback) : texture_name;
    }
    float ReadFloat(std::uint32_t document, std::string_view key,
                    float fallback) override {
        Check(document == 0x5000u, "float document");
        reads.emplace_back(key);
        auto it = floats.find(std::string(key));
        return it == floats.end() ? fallback : it->second;
    }
    std::int32_t ReadInt(std::uint32_t document, std::string_view key,
                         std::int32_t fallback) override {
        Check(document == 0x5000u, "integer document");
        reads.emplace_back(key);
        auto it = ints.find(std::string(key));
        return it == ints.end() ? fallback : it->second;
    }
    std::uint32_t FindTexture(std::string_view path) override {
        events.push_back("find-texture");
        found_path = path;
        return found_texture;
    }
    std::uint32_t LoadTexture(std::string_view path) override {
        events.push_back("load-texture");
        loaded_path = path;
        return loaded_texture;
    }
    void DestroyTexture(std::uint32_t texture) override {
        events.push_back("destroy-texture");
        destroyed_texture = texture;
    }
    std::uint32_t AllocateObject(std::uint32_t bytes_count) override {
        events.push_back("allocate");
        Check(bytes_count == 52u, "embedded object allocation size");
        return object;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        events.push_back("write-byte");
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        events.push_back("write-word");
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("read-word");
        return words.at(address);
    }
    bool InitializeObject(std::uint32_t address, std::uint32_t texture,
                          const std::array<float, 2>& initial_size,
                          std::uint8_t alpha_blend,
                          std::uint8_t axis_billboard,
                          std::uint8_t center) override {
        events.push_back("initialize");
        Check(address == object, "object initialization address");
        Check(texture == (found_texture == 0u ? loaded_texture : found_texture),
              "object receives acquired texture");
        size = initial_size;
        flags = {alpha_blend, axis_billboard, center};
        if (throw_initialize)
            throw std::runtime_error("initialize");
        if (initialized)
            words[address + 48u] = 0x9000u;
        return initialized;
    }
    void SetColor(std::uint32_t slot, std::uint32_t rgba) override {
        events.push_back("color");
        color_slot = slot;
        color = rgba;
    }
    void InitializeTransform(std::uint32_t address) override {
        events.push_back("transform");
        Check(address == object, "transform address");
        const auto position = floats.find("Attr_BillBord_Position_x");
        Check(words.at(address + 28u) ==
                  std::bit_cast<std::uint32_t>(
                      position == floats.end() ? 0.0f : position->second),
              "position written before transform initialization");
    }
    void AssignDocument(pal4::EffectSystemObjectFState& state,
                        std::uint32_t document) override {
        events.push_back("assign-document");
        Check(document == 0x5000u, "XML assignment source");
        state.words[14u] = 0xabcdu;
    }
};

void TestDefaultsAndOrder() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[2u] = 0xff000000u;
    Check(pal4::EffectSystemObjectF::LoadConfiguration(
              state, 0x5000u, runtime) == 1u,
          "successful load returns active flag");
    const std::vector<std::string> expected{
        "Attr_BillBord_Texture", "Attr_Auto_AlphaBlend",
        "Attr_BillBord_Width", "Attr_BillBord_Height",
        "Attr_BillBord_Scale_Width", "Attr_BillBord_Scale_Height",
        "Attr_BillBord_Rotate_x", "Attr_BillBord_Position_x",
        "Attr_BillBord_Position_y", "Attr_BillBord_Position_z",
        "Attr_BillBord_AxisBillbord", "Attr_BillBord_Color_Red",
        "Attr_BillBord_Color_Green", "Attr_BillBord_Color_Blue",
        "Attr_BillBord_Color_Alpha", "Attr_Render_Last", "Attr_Center"};
    Check(runtime.reads == expected, "all XML keys read in native order");
    Check(runtime.found_path == std::string(
              "gamedata\\effect\\tex\\\xCE\xB4\xC9\xE8\xD6\xC3") &&
              runtime.loaded_path.empty(),
          "default GBK texture path and no fallback load on hit");
    Check(runtime.size == std::array<float, 2>{16.0f, 16.0f} &&
              runtime.flags == std::array<std::uint8_t, 3>{1u, 0u, 1u},
          "runtime defaults differ from XML document creation defaults");
    Check(state.words[29u] == 0x6000u && state.words[31u] == 0xffffffffu &&
              state.words[32u] == 0u &&
              state.words[33u] == std::bit_cast<std::uint32_t>(1.0f) &&
              state.words[34u] == std::bit_cast<std::uint32_t>(1.0f) &&
              state.words[35u] == 0u && state.words[36u] == 0u &&
              state.words[37u] == 0u && state.words[14u] == 0xabcdu &&
              (state.words[1u] & 0xffu) == 1u &&
              state.words[2u] == 0x00010001u,
          "successful load writes state cache and flags");
    Check(runtime.bytes.at(0x600cu) == 0u &&
              runtime.bytes.at(0x600du) == 1u &&
              runtime.bytes.at(0x600eu) == 0u &&
              runtime.bytes.at(0x600fu) == 1u &&
              runtime.bytes.at(0x6010u) == 0u &&
              runtime.words.at(0x6014u) == 0u &&
              runtime.words.at(0x6018u) == 0u &&
              runtime.words.at(0x6020u) == 0u &&
              runtime.words.at(0x6030u) == 0x9000u &&
              runtime.color_slot == 0x9000u &&
              runtime.color == 0xffffffffu &&
              runtime.destroyed_texture == 0x8000u,
          "object prefix, color destination and texture release");
    Check(runtime.events.back() == "destroy-texture" &&
              runtime.events[runtime.events.size() - 2u] == "assign-document",
          "XML assignment precedes texture release");
}

void TestOverridesAndTextureFallback() {
    Runtime runtime;
    runtime.texture_name = "spark.dds";
    runtime.found_texture = 0u;
    runtime.floats = {{"Attr_BillBord_Width", 24.0f},
                      {"Attr_BillBord_Height", 48.0f},
                      {"Attr_BillBord_Rotate_x", 30.0f},
                      {"Attr_BillBord_Position_x", 1.0f},
                      {"Attr_BillBord_Position_y", 2.0f},
                      {"Attr_BillBord_Position_z", 3.0f},
                      {"Attr_BillBord_Scale_Width", 4.0f},
                      {"Attr_BillBord_Scale_Height", 5.0f}};
    runtime.ints = {{"Attr_Auto_AlphaBlend", -1},
                    {"Attr_BillBord_AxisBillbord", 3},
                    {"Attr_BillBord_Color_Red", 258},
                    {"Attr_BillBord_Color_Green", -1},
                    {"Attr_BillBord_Color_Blue", 3},
                    {"Attr_BillBord_Color_Alpha", 4},
                    {"Attr_Render_Last", 0},
                    {"Attr_Center", -3}};
    pal4::EffectSystemObjectFState state{};
    Check(pal4::EffectSystemObjectF::LoadConfiguration(
              state, 0x5000u, runtime) == 1u,
          "overridden configuration succeeds");
    Check(runtime.found_path == "gamedata\\effect\\tex\\spark.dds" &&
              runtime.loaded_path == runtime.found_path &&
              runtime.destroyed_texture == 0x8100u,
          "missing texture loads from same path and is released");
    Check(runtime.size == std::array<float, 2>{24.0f, 48.0f} &&
              runtime.flags == std::array<std::uint8_t, 3>{0u, 1u, 0u} &&
              runtime.color == 0x0403ff02u &&
              state.words[31u] == 0x0403ff02u &&
              state.words[32u] == std::bit_cast<std::uint32_t>(30.0f) &&
              state.words[33u] == std::bit_cast<std::uint32_t>(4.0f) &&
              state.words[34u] == std::bit_cast<std::uint32_t>(5.0f) &&
              state.words[35u] == std::bit_cast<std::uint32_t>(1.0f) &&
              state.words[36u] == std::bit_cast<std::uint32_t>(2.0f) &&
              state.words[37u] == std::bit_cast<std::uint32_t>(3.0f) &&
              state.words[2u] == 0u,
          "overrides preserve values, positive booleans and color byte truncation");
    Check(runtime.words.at(0x6018u) == std::bit_cast<std::uint32_t>(30.0f) &&
              runtime.words.at(0x601cu) == std::bit_cast<std::uint32_t>(1.0f) &&
              runtime.words.at(0x6020u) == std::bit_cast<std::uint32_t>(2.0f) &&
              runtime.words.at(0x6024u) == std::bit_cast<std::uint32_t>(3.0f) &&
              runtime.words.at(0x6028u) == std::bit_cast<std::uint32_t>(4.0f) &&
              runtime.words.at(0x602cu) == std::bit_cast<std::uint32_t>(5.0f),
          "object rotation, position and scale offsets");
}

void TestFailureBoundaries() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[1u] = 0xaabbcc55u;
    Check(pal4::EffectSystemObjectF::LoadConfiguration(
              state, 0u, runtime) == 0u &&
              state.words[1u] == 0xaabbcc55u &&
              runtime.events.empty() && runtime.reads.empty(),
          "null document returns without changing state");

    runtime.initialized = false;
    state.words[1u] = 0u;
    state.words[2u] = 0xff000000u;
    Check(pal4::EffectSystemObjectF::LoadConfiguration(
              state, 0x5000u, runtime) == 0u &&
              state.words[29u] == 0x6000u &&
              state.words[2u] == 0x00010001u &&
              state.words[31u] == 0u &&
              runtime.destroyed_texture == 0x8000u,
          "failed object initialization retains allocated pointer and parsed flags");
    Check(runtime.color_slot == 0u && state.words[14u] == 0u,
          "failed initialization skips color and XML assignment");

    runtime = Runtime{};
    runtime.throw_initialize = true;
    try {
        static_cast<void>(pal4::EffectSystemObjectF::LoadConfiguration(
            state, 0x5000u, runtime));
        Check(false, "injected exception must escape");
    } catch (const std::runtime_error&) {
        Check(runtime.destroyed_texture == 0x8000u,
              "exception releases acquired texture");
    }
}

void TestReapplyCachedConfiguration() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[2u] = 0xee000000u;
    Check(pal4::EffectSystemObjectF::ReapplyCachedConfiguration(
              state, runtime) == 0u && runtime.events.empty() &&
              state.words[2u] == 0xee000000u,
          "inactive object skips cached settings");

    runtime.floats["Attr_BillBord_Position_x"] = 1.0f;
    runtime.floats["Attr_BillBord_Position_y"] = 2.0f;
    runtime.floats["Attr_BillBord_Position_z"] = 3.0f;
    static_cast<void>(pal4::EffectSystemObjectF::LoadConfiguration(
        state, 0x5000u, runtime));
    state.words[2u] |= 0xee000000u;
    runtime.events.clear();
    runtime.words[0x6018u] = 0u;
    runtime.words[0x601cu] = 0u;
    runtime.words[0x6020u] = 0u;
    runtime.words[0x6024u] = 0u;
    runtime.words[0x6028u] = 0u;
    runtime.words[0x602cu] = 0u;

    Check(pal4::EffectSystemObjectF::ReapplyCachedConfiguration(
              state, runtime) == 1u && state.words[2u] == 0x00010001u,
          "active reapply clears only timed mode byte");
    Check(runtime.events == std::vector<std::string>{
              "read-word", "color", "write-word", "write-word",
              "write-word", "write-word", "write-word", "write-word",
              "transform"},
          "cached color and scalar writes precede transform");
    Check(runtime.words.at(0x6018u) == state.words[32u] &&
              runtime.words.at(0x6028u) == state.words[33u] &&
              runtime.words.at(0x602cu) == state.words[34u] &&
              runtime.words.at(0x601cu) == state.words[35u] &&
              runtime.words.at(0x6020u) == state.words[36u] &&
              runtime.words.at(0x6024u) == state.words[37u],
          "cached rotation, scale and position use native offsets");
}

}  // namespace

int main() {
    TestDefaultsAndOrder();
    TestOverridesAndTextureFallback();
    TestFailureBoundaries();
    TestReapplyCachedConfiguration();
}
