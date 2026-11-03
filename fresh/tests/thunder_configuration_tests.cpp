#include "pal4/thunder_configuration.h"

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

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Bits(const float value) {
    return std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::IThunderConfigurationRuntime {
    std::map<std::string, std::string> strings;
    std::map<std::string, std::int32_t> ints;
    std::map<std::string, float> floats;
    std::vector<std::string> reads;
    std::vector<std::string> events;
    std::uint32_t found_texture = 0x3400u;
    std::uint32_t loaded_texture = 0x4500u;
    bool visual_success = true;
    bool throw_on_visual = false;
    std::array<float, 3u> resolved_target{9.0f, 8.0f, 7.0f};
    std::uint32_t update_count = 0u;
    std::vector<std::array<std::uint32_t, 9u>> frame_snapshots;

    std::string ReadString(const std::uint32_t document,
                           const std::string_view key,
                           const std::string_view fallback) override {
        Check(document == 0x1000u, "string document handle");
        reads.emplace_back(key);
        const auto found = strings.find(std::string(key));
        return found == strings.end() ? std::string(fallback) : found->second;
    }
    std::int32_t ReadInt(const std::uint32_t document,
                         const std::string_view key,
                         const std::int32_t fallback) override {
        Check(document == 0x1000u, "integer document handle");
        reads.emplace_back(key);
        const auto found = ints.find(std::string(key));
        return found == ints.end() ? fallback : found->second;
    }
    float ReadFloat(const std::uint32_t document,
                    const std::string_view key,
                    const float fallback) override {
        Check(document == 0x1000u, "float document handle");
        reads.emplace_back(key);
        const auto found = floats.find(std::string(key));
        return found == floats.end() ? fallback : found->second;
    }
    void SetTargetName(pal4::ThunderObjectState&,
                       const std::string_view name) override {
        events.emplace_back("name:" + std::string(name));
    }
    void ResolveTargetPosition(const std::uint32_t owner,
                               const std::string_view name,
                               std::array<float, 3u>& position) override {
        Check(owner == 0x9000u && name == "Enemy",
              "target lookup owner and name");
        events.emplace_back("target");
        position = resolved_target;
    }
    std::uint32_t FindTexture(const std::string_view path) override {
        Check(path == "gamedata\\effect\\tex\\Bolt.png",
              "native texture path");
        events.emplace_back("find");
        return found_texture;
    }
    std::uint32_t LoadTexture(const std::string_view path) override {
        Check(path == "gamedata\\effect\\tex\\Bolt.png",
              "fallback texture path");
        events.emplace_back("load");
        return loaded_texture;
    }
    bool InitializeVisual(pal4::ThunderObjectState& state,
                          const std::uint32_t texture,
                          const float fps,
                          const std::uint8_t mode) override {
        Check(texture == (found_texture == 0u ? loaded_texture : found_texture) &&
                  mode == 0u && fps ==
                  (floats.contains("Attr_Thunder_Fps") ? 24.0f : 0.0f),
              "visual initialization texture, fps and mode");
        Check((state.words[58u] & 0xffu) ==
                  (ints.contains("Attr_Thunder_Local_Model") ? 1u : 0u),
              "local model byte written before visual initialization");
        events.emplace_back("visual");
        if (throw_on_visual) throw std::runtime_error("visual");
        if (visual_success) {
            state.words[28u] = 0x7777u;
            state.words[33u] = 0x8888u;
        }
        return visual_success;
    }
    void UpdateFrame(pal4::ThunderObjectState& state) override {
        ++update_count;
        Check(state.words[28u] == 0x7777u && state.words[33u] == 0x8888u,
              "visual frame initialized before transform writes");
        events.emplace_back("frame");
        std::array<std::uint32_t, 9u> snapshot{};
        for (std::uint32_t i = 0u; i < snapshot.size(); ++i)
            snapshot[i] = state.words[18u + i];
        frame_snapshots.push_back(snapshot);
    }
    void DestroyTexture(const std::uint32_t texture) override {
        Check(texture == (found_texture == 0u ? loaded_texture : found_texture),
              "released texture handle");
        events.emplace_back("destroy");
    }
    void ReleaseTexturePath(const std::string_view path) override {
        Check(path == "gamedata\\effect\\tex\\Bolt.png",
              "released native texture path");
        events.emplace_back("path");
    }
};

void CheckTriple(const pal4::ThunderObjectState& state,
                 const std::uint32_t offset,
                 const std::array<float, 3u>& expected,
                 const char* message) {
    for (std::uint32_t i = 0u; i < 3u; ++i)
        Check(state.words[offset / 4u + i] == Bits(expected[i]), message);
}

void TestConfiguredVisual() {
    pal4::ThunderObjectState state{};
    state.words[3u] = 0x9000u;
    state.words[2u] = 0xffabcdefu;
    state.words[58u] = 0x11223344u;
    Runtime runtime;
    runtime.strings = {{"Attr_Thunder_Texture", "Bolt.png"},
                       {"Attr_Thunder_Target_Name", "Enemy"}};
    runtime.ints = {{"Attr_Thunder_AlphaBlend", 2},
                    {"Attr_Thunder_Lock_Target", 1},
                    {"Attr_Thunder_Size_Gradual", 1},
                    {"Attr_Thunder_Range_Gradual", 1},
                    {"Attr_Thunder_Move_Target", 1},
                    {"Attr_Thunder_Sample", 1},
                    {"Attr_Thunder_Sample_Coeff", 9},
                    {"Attr_Thunder_Color_Red", 12},
                    {"Attr_Thunder_Color_Green", 34},
                    {"Attr_Thunder_Color_Blue", 56},
                    {"Attr_Thunder_Color_Alpha", 78}};
    runtime.floats = {
        {"Attr_Thunder_Target_x", 1.0f},
        {"Attr_Thunder_Target_y", 2.0f},
        {"Attr_Thunder_Target_z", 3.0f},
        {"Attr_Thunder_Fps", 24.0f},
        {"Attr_Thunder_MinRange", 0.12f},
        {"Attr_Thunder_MaxRange", 0.34f},
        {"Attr_Thunder_Section_Min_Width", 1.0f},
        {"Attr_Thunder_Section_Max_Width", 2.0f},
        {"Attr_Thunder_Section_Min_Height", 3.0f},
        {"Attr_Thunder_Section_Max_Height", 4.0f},
        {"Attr_Thunder_Scale_x", 2.0f},
        {"Attr_Thunder_Scale_y", 3.0f},
        {"Attr_Thunder_Scale_z", 4.0f},
        {"Attr_Thunder_Rotate_x", 5.0f},
        {"Attr_Thunder_Rotate_y", 6.0f},
        {"Attr_Thunder_Rotate_z", 7.0f},
        {"Attr_Thunder_Position_x", 8.0f},
        {"Attr_Thunder_Position_y", 9.0f},
        {"Attr_Thunder_Position_z", 10.0f}};

    Check(pal4::ThunderConfiguration::Load(state, 0x1000u, runtime) == 1u,
          "configured document loads");
    Check(runtime.events == std::vector<std::string>{
              "name:Default", "name:Enemy", "target", "find", "visual",
              "frame", "frame", "frame", "destroy", "path"},
          "target, texture, frame and cleanup order");
    const std::vector<std::string> expected_reads{
        "Attr_Thunder_Texture", "Attr_Thunder_Target_Name",
        "Attr_Thunder_AlphaBlend", "Attr_Thunder_Local_Model",
        "Attr_Thunder_Lock_Target", "Attr_Thunder_Size_Gradual",
        "Attr_Thunder_Range_Gradual", "Attr_Thunder_Move_Target",
        "Attr_Thunder_Target_x", "Attr_Thunder_Target_y",
        "Attr_Thunder_Target_z", "Attr_Thunder_Fps",
        "Attr_Thunder_MinRange", "Attr_Thunder_MaxRange",
        "Attr_Thunder_Section_Min_Width", "Attr_Thunder_Section_Max_Width",
        "Attr_Thunder_Section_Min_Height", "Attr_Thunder_Section_Max_Height",
        "Attr_Thunder_Position_x", "Attr_Thunder_Position_y",
        "Attr_Thunder_Position_z", "Attr_Thunder_Scale_x",
        "Attr_Thunder_Scale_y", "Attr_Thunder_Scale_z",
        "Attr_Thunder_Rotate_x", "Attr_Thunder_Rotate_y",
        "Attr_Thunder_Rotate_z", "Attr_Thunder_Color_Red",
        "Attr_Thunder_Color_Green", "Attr_Thunder_Color_Blue",
        "Attr_Thunder_Color_Alpha", "Attr_Thunder_Sample",
        "Attr_Thunder_Sample_Coeff"};
    Check(runtime.update_count == 3u && runtime.reads == expected_reads,
          "all thirty-three configuration properties read in IDA order");
    Check(runtime.frame_snapshots == std::vector<std::array<std::uint32_t, 9u>>{
              {Bits(2.0f), Bits(3.0f), Bits(4.0f), 0u, 0u, 0u,
               0u, 0u, 0u},
              {Bits(2.0f), Bits(3.0f), Bits(4.0f), Bits(5.0f),
               Bits(6.0f), Bits(7.0f), 0u, 0u, 0u},
              {Bits(2.0f), Bits(3.0f), Bits(4.0f), Bits(5.0f),
               Bits(6.0f), Bits(7.0f), Bits(8.0f), Bits(9.0f),
               Bits(10.0f)}},
          "frame refreshes after scale, rotation and position separately");
    CheckTriple(state, 72u, {2.0f, 3.0f, 4.0f}, "scale offset +72");
    CheckTriple(state, 84u, {5.0f, 6.0f, 7.0f}, "rotation offset +84");
    CheckTriple(state, 96u, {8.0f, 9.0f, 10.0f}, "position offset +96");
    CheckTriple(state, 148u, {9.0f, 8.0f, 7.0f}, "resolved target +148");
    CheckTriple(state, 244u, {2.0f, 3.0f, 4.0f}, "cached scale +244");
    CheckTriple(state, 256u, {5.0f, 6.0f, 7.0f}, "cached rotation +256");
    CheckTriple(state, 268u, {8.0f, 9.0f, 10.0f}, "cached position +268");
    CheckTriple(state, 280u, {9.0f, 8.0f, 7.0f}, "cached target +280");
    Check(state.words[40u] == Bits(0.12f) &&
              state.words[41u] == Bits(0.34f) &&
              state.words[42u] == Bits(1.0f) &&
              state.words[45u] == Bits(4.0f),
          "range and section defaults at +160..+180");
    Check(state.words[27u] == 0x4e38220cu &&
              state.words[60u] == state.words[27u] &&
              (state.words[58u] & 0xffu) == 0u &&
              (state.words[14u] & 0xffu) == 1u &&
              (state.words[1u] & 0xffu) == 1u &&
              (state.words[2u] >> 24u) == 0u,
          "packed color, flags, activity and playback reset");
    Check(((state.words[15u] >> 8u) & 0xffffffu) == 0x010001u &&
              ((state.words[16u] & 0xffffffu) == 0x10101u) &&
              state.words[17u] == 9u && state.words[58u] == 0x11223300u,
          "alpha, lock, gradual, sample, coefficient and byte isolation");
}

void TestLocalAndFallback() {
    pal4::ThunderObjectState state{};
    state.words[3u] = 0x9000u;
    Runtime runtime;
    runtime.found_texture = 0u;
    runtime.strings = {{"Attr_Thunder_Texture", "Bolt.png"},
                       {"Attr_Thunder_Target_Name", "Enemy"}};
    runtime.ints["Attr_Thunder_Local_Model"] = 1;
    runtime.floats = {{"Attr_Thunder_Target_x", 1.0f},
                      {"Attr_Thunder_Target_y", 2.0f},
                      {"Attr_Thunder_Target_z", 3.0f}};
    Check(pal4::ThunderConfiguration::Load(state, 0x1000u, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "name:Default", "name:Enemy", "find", "load", "visual",
                  "frame", "frame", "frame", "destroy", "path"},
          "local model skips target lookup and loads missing texture");
    CheckTriple(state, 148u, {1.0f, 2.0f, 3.0f},
                "local model retains explicit target");
    CheckTriple(state, 72u, {1.0f, 1.0f, 1.0f},
                "scale defaults to unit vector");
    Check(state.words[27u] == 0xb4ffffffu &&
              ((state.words[15u] >> 16u) & 0xffu) == 1u &&
              (state.words[58u] & 0xffu) == 1u && state.words[17u] == 4u,
          "native color, local-model and sample-coefficient defaults");
}

void TestFailurePaths() {
    pal4::ThunderObjectState state{};
    Runtime runtime;
    Check(pal4::ThunderConfiguration::Load(state, 0u, runtime) == 0u &&
              runtime.events.empty() && runtime.reads.empty(),
          "null document skips all work");

    runtime.strings["Attr_Thunder_Texture"] = "Bolt.png";
    runtime.found_texture = 0u;
    runtime.loaded_texture = 0u;
    runtime.visual_success = false;
    Check(pal4::ThunderConfiguration::Load(state, 0x1000u, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "name:Default", "name:", "find", "load", "visual", "path"} &&
              (state.words[1u] & 0xffu) == 0u,
          "missing texture still returns one and leaves object inactive");

    Runtime visual_failure;
    visual_failure.strings["Attr_Thunder_Texture"] = "Bolt.png";
    visual_failure.visual_success = false;
    Check(pal4::ThunderConfiguration::Load(state, 0x1000u,
                                           visual_failure) == 1u &&
              visual_failure.events == std::vector<std::string>{
                  "name:Default", "name:", "find", "visual", "destroy",
                  "path"},
          "visual initialization failure releases acquired texture");

    Runtime exception;
    exception.strings["Attr_Thunder_Texture"] = "Bolt.png";
    exception.throw_on_visual = true;
    bool caught = false;
    try {
        (void)pal4::ThunderConfiguration::Load(state, 0x1000u, exception);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && exception.events == std::vector<std::string>{
              "name:Default", "name:", "find", "visual", "destroy", "path"},
          "exception releases texture and path once");
}

}  // namespace

int main() {
    TestConfiguredVisual();
    TestLocalAndFallback();
    TestFailurePaths();
}
