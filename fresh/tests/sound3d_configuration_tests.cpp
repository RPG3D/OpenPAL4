#include "pal4/sound3d_configuration.h"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
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

struct Runtime final : pal4::ISound3DConfigurationRuntime {
    std::vector<std::string> events;
    std::array<float, 3> position{2.5f, -3.0f, 4.25f};
    std::string filename = "sound.wav";
    std::int32_t mode_2d = 1;
    bool fail_string = false;

    float ReadFloat(std::uint32_t document, std::string_view key,
                    float fallback) override {
        Check(document == 0x1000u && fallback == 0.0f,
              "float XML document and default");
        constexpr std::array<std::string_view, 3> keys{
            "Attr_Sound3D_SoundPos_x", "Attr_Sound3D_SoundPos_y",
            "Attr_Sound3D_SoundPos_z"};
        const auto index = events.size();
        Check(index < keys.size() && key == keys[index],
              "position attributes read in native order");
        events.emplace_back(std::string(key));
        return position[index];
    }
    std::string ReadString(std::uint32_t document,
                           std::string_view key) override {
        Check(document == 0x1000u && key == "Attr_Sound3D_SoundFile",
              "sound filename XML key");
        events.emplace_back("read-file");
        if (fail_string) throw std::runtime_error("read-file");
        return filename;
    }
    std::int32_t ReadInt(std::uint32_t document, std::string_view key,
                         std::int32_t fallback) override {
        Check(document == 0x1000u && key == "Attr_Sound3D_2D" &&
                  fallback == 0, "2D flag XML key and default");
        events.emplace_back("read-2d");
        return mode_2d;
    }
    void InitializeSoundPath(pal4::Sound3DObjectState&,
                             std::string_view prefix) override {
        Check(prefix == "gamedata\\PALSound\\", "native sound path prefix");
        events.emplace_back("prefix");
    }
    void AppendSoundFile(pal4::Sound3DObjectState&,
                         std::string_view value) override {
        Check(value == filename, "read filename appended");
        events.emplace_back("append-file");
    }
    std::uint32_t CreateFrame() override {
        events.emplace_back("create-frame");
        return 0x5000u;
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x5000u, "identity frame");
        events.emplace_back("identity");
    }
    void ScaleFrame(std::uint32_t frame, const std::array<float, 3>& scale,
                    std::uint32_t mode) override {
        Check(frame == 0x5000u && scale == std::array<float, 3>{1, 1, 1} &&
                  mode == 0u, "unit frame scale in replace mode");
        events.emplace_back("scale");
    }
    void RotateFrame(std::uint32_t frame, const std::array<float, 3>& axis,
                     float degrees, std::uint32_t mode) override {
        constexpr std::array<std::array<float, 3>, 3> axes{{
            {1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
        const auto index = events.size() - 10u;
        Check(frame == 0x5000u && index < axes.size() &&
                  axis == axes[index] && degrees == 0.0f && mode == 2u,
              "three zero rotations use native axes and combine mode");
        events.emplace_back("rotate");
    }
    void TranslateFrame(std::uint32_t frame,
                        const std::array<float, 3>& value,
                        std::uint32_t mode) override {
        Check(frame == 0x5000u && value == position && mode == 2u,
              "frame translated by read position");
        events.emplace_back("translate");
    }
};

void TestLoad() {
    pal4::Sound3DObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    Check(pal4::Sound3DConfiguration::Load(state, 0x1000u, runtime) == 1u,
          "valid document succeeds");
    for (std::uint32_t i = 0u; i < 3u; ++i)
        Check(state.words[10u + i] == std::bit_cast<std::uint32_t>(runtime.position[i]) &&
                  state.words[20u + i] == state.words[10u + i],
              "position words and cached copy");
    Check(state.words[13u] == 0x5000u &&
              (state.words[14u] & 0xffu) == 1u &&
              (state.words[1u] & 0xffu) == 1u &&
              ((state.words[2u] >> 24u) & 0xffu) == 0u,
          "frame, 2D, activity and playback fields");
    Check(runtime.events == std::vector<std::string>{
              "Attr_Sound3D_SoundPos_x", "Attr_Sound3D_SoundPos_y",
              "Attr_Sound3D_SoundPos_z", "prefix", "read-file",
              "append-file", "read-2d", "create-frame", "identity",
              "scale", "rotate", "rotate", "rotate", "translate"},
          "native read, string and frame order");

    runtime.events.clear();
    runtime.mode_2d = -1;
    Check(pal4::Sound3DConfiguration::Load(state, 0x1000u, runtime) == 1u &&
              (state.words[14u] & 0xffu) == 0u,
          "nonpositive 2D setting clears byte");
}

void TestFailureBoundaries() {
    pal4::Sound3DObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    Check(pal4::Sound3DConfiguration::Load(state, 0u, runtime) == 0u &&
              runtime.events.empty() && state.words[13u] == 0xaaaaaaaau,
          "null document leaves object untouched");
    runtime.fail_string = true;
    try {
        (void)pal4::Sound3DConfiguration::Load(state, 0x1000u, runtime);
        Check(false, "string read exception expected");
    } catch (const std::runtime_error&) {
        Check(runtime.events == std::vector<std::string>{
                  "Attr_Sound3D_SoundPos_x", "Attr_Sound3D_SoundPos_y",
                  "Attr_Sound3D_SoundPos_z", "prefix", "read-file"} &&
                  state.words[13u] == 0xaaaaaaaau,
              "failed filename read does not create frame");
    }
}

}  // namespace

int main() {
    TestLoad();
    TestFailureBoundaries();
}
