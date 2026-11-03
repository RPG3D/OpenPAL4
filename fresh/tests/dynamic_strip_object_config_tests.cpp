#include "pal4/dynamic_strip_object_config.h"

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

std::array<float, 3> VectorAt(
    const pal4::DynamicStripObjectState& state, const std::uint32_t offset) {
    return {
        std::bit_cast<float>(state.words[offset / 4u]),
        std::bit_cast<float>(state.words[offset / 4u + 1u]),
        std::bit_cast<float>(state.words[offset / 4u + 2u])};
}

struct Runtime final : pal4::IDynamicStripObjectConfigRuntime {
    std::vector<std::string> calls;
    std::map<std::string, std::int32_t> ints;
    std::map<std::string, float> floats;
    std::uint32_t found_texture = 0u;
    std::uint32_t loaded_texture = 0x4000u;
    bool setup_result = true;
    bool fail_on_find = false;
    bool validate_configured = true;
    std::uint32_t frame_updates = 0u;
    std::uint32_t collection_cleanups = 0u;
    std::uint32_t visual_texture = 0u;
    float visual_width = 0.0f;
    float visual_life = 0.0f;
    std::string last_path;

    std::string_view ReadTextureName(std::uint32_t document,
                                     std::string_view key,
                                     std::uint32_t default_address) override {
        Check(document == 0x2000u &&
                  key == "Attr_StripSystem_Texture" &&
                  default_address == 0x8e425cu,
              "texture reader receives native key and default address");
        calls.push_back("texture_name");
        return "strip.dds";
    }
    std::int32_t ReadInt(std::uint32_t document,
                         std::string_view key,
                         std::int32_t default_value) override {
        Check(document == 0x2000u, "int reader document");
        calls.push_back("int:" + std::string(key));
        const auto found = ints.find(std::string(key));
        return found == ints.end() ? default_value : found->second;
    }
    float ReadFloat(std::uint32_t document,
                    std::string_view key,
                    float default_value) override {
        Check(document == 0x2000u, "float reader document");
        calls.push_back("float:" + std::string(key));
        const auto found = floats.find(std::string(key));
        return found == floats.end() ? default_value : found->second;
    }
    std::uint32_t FindTexture(std::string_view path) override {
        Check(path == "gamedata\\effect\\tex\\strip.dds",
              "texture dictionary path");
        calls.push_back("find_texture");
        last_path = path;
        if (fail_on_find)
            throw std::runtime_error("find");
        return found_texture;
    }
    std::uint32_t LoadTexture(std::string_view path) override {
        Check(path == last_path, "texture loader receives same path");
        calls.push_back("load_texture");
        return loaded_texture;
    }
    void UpdateFrameProperties(
        pal4::DynamicStripObjectState& state) override {
        ++frame_updates;
        calls.push_back("update_frame");
        if (validate_configured && frame_updates == 1u)
            Check(VectorAt(state, 68u) ==
                      std::array<float, 3>{2.0f, 3.0f, 4.0f},
                  "first frame update sees model scale");
        if (validate_configured && frame_updates == 2u)
            Check(VectorAt(state, 80u) ==
                      std::array<float, 3>{10.0f, 20.0f, 30.0f},
                  "second frame update sees rotation");
        if (validate_configured && frame_updates == 3u)
            Check(VectorAt(state, 92u) ==
                      std::array<float, 3>{5.0f, 6.0f, 7.0f},
                  "third frame update sees position");
    }
    void CleanupCollection(pal4::DynamicStripObjectState& state) override {
        Check(frame_updates == 3u &&
                  ((state.words[2u] >> 24u) & 0xffu) == 0u,
              "collection cleanup follows transform and mode reset");
        ++collection_cleanups;
        calls.push_back("cleanup_collection");
    }
    bool VisualSetup(pal4::DynamicStripObjectState& state,
                     std::uint32_t texture, float width,
                     float life) override {
        calls.push_back("visual_setup");
        visual_texture = texture;
        visual_width = width;
        visual_life = life;
        Check(frame_updates == 3u && state.words[16u] == 0u,
              "setup follows three frame updates and FPS reset");
        if (validate_configured)
            Check(state.words[26u] == 0xfe03ff01u &&
                      ((state.words[13u] >> 8u) & 0xffu) == 1u &&
                      (state.words[14u] & 0xffu) == 1u &&
                      std::bit_cast<float>(state.words[15u]) == 0.25f,
                  "configured color, alpha, sample and FPS reach setup");
        if (setup_result)
            state.words[28u] = std::bit_cast<std::uint32_t>(life);
        return setup_result;
    }
    void ReleaseTexture(std::uint32_t texture) override {
        Check(texture == visual_texture && texture != 0u,
              "temporary texture reference released");
        calls.push_back("release_texture");
    }
    void ReleaseTexturePath(std::string_view path) override {
        Check(path == "gamedata\\effect\\tex\\strip.dds",
              "temporary path released");
        calls.push_back("release_path");
    }
};

Runtime ConfiguredRuntime() {
    Runtime runtime;
    runtime.ints = {
        {"Attr_StripSystem_AlphaBlend", 3},
        {"Attr_StripSystem_Color_Red", 257},
        {"Attr_StripSystem_Color_Green", 255},
        {"Attr_StripSystem_Color_Blue", 259},
        {"Attr_StripSystem_Color_Alpha", 254},
        {"Attr_StripSystem_Fps", 4},
        {"Attr_StripSystem_Sample", 2}};
    runtime.floats = {
        {"Attr_StripSystem_Life", 12.0f},
        {"Attr_StripSystem_Width", 2.5f},
        {"Attr_StripSystem_Position_x", 5.0f},
        {"Attr_StripSystem_Position_y", 6.0f},
        {"Attr_StripSystem_Position_z", 7.0f},
        {"Attr_StripSystem_Scale_x", 2.0f},
        {"Attr_StripSystem_Scale_y", 3.0f},
        {"Attr_StripSystem_Scale_z", 4.0f},
        {"Attr_StripSystem_Rotate_x", 10.0f},
        {"Attr_StripSystem_Rotate_y", 20.0f},
        {"Attr_StripSystem_Rotate_z", 30.0f}};
    return runtime;
}

void TestSuccess() {
    pal4::DynamicStripObjectState state{};
    Runtime runtime = ConfiguredRuntime();
    using pal4::DynamicStripObjectConfiguration;
    Check(DynamicStripObjectConfiguration::Load(state, 0u, runtime) == 0u &&
              runtime.calls.empty(),
          "null XML document exits without native calls");
    Check(DynamicStripObjectConfiguration::Load(state, 0x2000u, runtime) == 1u &&
              state.words[47u] == 0xfe03ff01u &&
              VectorAt(state, 192u) ==
                  std::array<float, 3>{10.0f, 20.0f, 30.0f} &&
              VectorAt(state, 204u) ==
                  std::array<float, 3>{2.0f, 3.0f, 4.0f} &&
              VectorAt(state, 216u) ==
                  std::array<float, 3>{5.0f, 6.0f, 7.0f} &&
              (state.words[1u] & 0xffu) == 1u &&
              ((state.words[2u] >> 24u) & 0xffu) == 0u &&
              runtime.visual_texture == 0x4000u &&
              runtime.visual_width == 2.5f &&
              runtime.visual_life == 12.0f &&
              pal4::DynamicStripObject::GetLife(state) == 12.0f,
          "successful setup caches color/transforms and activates object");
    Check(runtime.calls.front() == "texture_name" &&
              runtime.calls[runtime.calls.size() - 6u] == "update_frame" &&
              runtime.calls[runtime.calls.size() - 5u] == "update_frame" &&
              runtime.calls[runtime.calls.size() - 4u] == "update_frame" &&
              runtime.calls[runtime.calls.size() - 3u] == "visual_setup" &&
              runtime.calls[runtime.calls.size() - 2u] == "release_texture" &&
              runtime.calls.back() == "release_path",
          "native frame, visual and cleanup calls retain order");
}

void TestFailureAndTextureHit() {
    pal4::DynamicStripObjectState state{};
    state.words.fill(0xa5a5a5a5u);
    state.words[1u] &= 0xffffff00u;
    Runtime runtime = ConfiguredRuntime();
    runtime.found_texture = 0x6000u;
    runtime.setup_result = false;
    Check(pal4::DynamicStripObjectConfiguration::Load(
              state, 0x2000u, runtime) == 1u &&
              runtime.visual_texture == 0x6000u &&
              (state.words[1u] & 0xffu) == 0u &&
              state.words[47u] == 0xa5a5a5a5u &&
              state.words[48u] == 0xa5a5a5a5u &&
              runtime.calls[runtime.calls.size() - 2u] ==
                  "release_texture" &&
              runtime.calls.back() == "release_path",
          "failed setup preserves activity and cache, but returns native one");
    for (const auto& call : runtime.calls)
        Check(call != "load_texture", "texture hit skips native load");
}

void TestMissingAttributes() {
    pal4::DynamicStripObjectState state{};
    Runtime runtime;
    runtime.validate_configured = false;
    runtime.loaded_texture = 0u;
    runtime.setup_result = false;
    Check(pal4::DynamicStripObjectConfiguration::Load(
              state, 0x2000u, runtime) == 1u &&
              runtime.visual_texture == 0u &&
              runtime.visual_width == 0.0f && runtime.visual_life == 0.0f &&
              VectorAt(state, 68u) ==
                  std::array<float, 3>{1.0f, 1.0f, 1.0f} &&
              VectorAt(state, 80u) ==
                  std::array<float, 3>{0.0f, 0.0f, 0.0f} &&
              VectorAt(state, 92u) ==
                  std::array<float, 3>{0.0f, 0.0f, 0.0f} &&
              state.words[26u] == 0xffffffffu &&
              ((state.words[13u] >> 8u) & 0xffu) == 0u &&
              (state.words[14u] & 0xffu) == 0u &&
              state.words[15u] == 0u && state.words[16u] == 0u &&
              (state.words[1u] & 0xffu) == 0u &&
              runtime.calls.back() == "release_path",
          "missing attributes use native loader defaults and zero FPS");
    for (const auto& call : runtime.calls)
        Check(call != "release_texture",
              "null texture has no temporary reference to release");
}

void TestExceptionCleanup() {
    pal4::DynamicStripObjectState state{};
    Runtime runtime = ConfiguredRuntime();
    runtime.fail_on_find = true;
    try {
        static_cast<void>(pal4::DynamicStripObjectConfiguration::Load(
            state, 0x2000u, runtime));
        Check(false, "texture lookup failure must throw");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "release_path" &&
                  runtime.frame_updates == 0u,
              "native unwind releases path before frame mutations");
    }
}

void TestReset() {
    pal4::DynamicStripObjectState state{};
    Runtime runtime = ConfiguredRuntime();
    using pal4::DynamicStripObjectConfiguration;
    state.words[26u] = 0x12345678u;
    Check(DynamicStripObjectConfiguration::Reset(state, runtime) == 0u &&
              state.words[26u] == 0x12345678u && runtime.calls.empty(),
          "inactive reset skips cache and callbacks");
    Check(DynamicStripObjectConfiguration::Load(state, 0x2000u, runtime) == 1u,
          "configured object loads before reset");
    state.words[26u] = 0u;
    state.words[20u] = 0u;
    state.words[17u] = 0u;
    state.words[23u] = 0u;
    state.words[2u] |= 0x7f000000u;
    runtime.calls.clear();
    runtime.frame_updates = 0u;
    runtime.validate_configured = false;
    Check(DynamicStripObjectConfiguration::Reset(state, runtime) == 1u &&
              state.words[26u] == 0xfe03ff01u &&
              VectorAt(state, 80u) ==
                  std::array<float, 3>{10.0f, 20.0f, 30.0f} &&
              VectorAt(state, 68u) ==
                  std::array<float, 3>{2.0f, 3.0f, 4.0f} &&
              VectorAt(state, 92u) ==
                  std::array<float, 3>{5.0f, 6.0f, 7.0f} &&
              ((state.words[2u] >> 24u) & 0xffu) == 0u &&
              runtime.collection_cleanups == 1u &&
              runtime.calls == std::vector<std::string>{
                  "update_frame", "update_frame", "update_frame",
                  "cleanup_collection"},
          "reset restores cache in native order and clears collection");
}

}  // namespace

int main() {
    TestSuccess();
    TestFailureAndTextureHit();
    TestMissingAttributes();
    TestExceptionCleanup();
    TestReset();
}
