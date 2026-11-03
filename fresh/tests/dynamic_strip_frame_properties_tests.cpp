#include "pal4/dynamic_strip_object_config.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void Float(pal4::DynamicStripObjectState& state,
           std::uint32_t offset, float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::IDynamicStripVisualRuntime {
    std::vector<std::string> calls;
    pal4::DynamicStripVector scale{}, translation{};
    std::vector<pal4::DynamicStripVector> axes;
    std::vector<float> angles;
    void CleanupResources(pal4::DynamicStripObjectState&) override {}
    void TextureSize(std::uint32_t, std::uint32_t&, std::uint32_t&) override {}
    void AddTextureReference(std::uint32_t) override {}
    std::uint32_t CreateFrame() override { return 0u; }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x800u, "identity frame");
        calls.push_back("identity");
    }
    void ScaleFrame(std::uint32_t frame, pal4::DynamicStripVector value,
                    int mode) override {
        Check(frame == 0x800u && mode == 0, "scale frame and mode");
        calls.push_back("scale");
        scale = value;
    }
    void RotateFrame(std::uint32_t frame, pal4::DynamicStripVector axis,
                     float angle, int mode) override {
        Check(frame == 0x800u && mode == 2, "rotation frame and mode");
        calls.push_back("rotate");
        axes.push_back(axis);
        angles.push_back(angle);
    }
    void TranslateFrame(std::uint32_t frame, pal4::DynamicStripVector value,
                        int mode) override {
        Check(frame == 0x800u && mode == 2, "translation frame and mode");
        calls.push_back("translate");
        translation = value;
    }
    void UpdateFrameObjects(std::uint32_t frame) override {
        Check(frame == 0x800u, "update frame");
        calls.push_back("update");
    }
    void AddChildFrame(std::uint32_t, std::uint32_t) override {}
    void UpdateFrameProperties(pal4::DynamicStripObjectState&) override {}
};

}  // namespace

int main() {
    pal4::DynamicStripObjectState state;
    Runtime runtime;
    Float(state, 16u, -1.0f);
    Float(state, 20u, 2.0f);
    Float(state, 24u, -3.0f);
    state.words[14u] = 0x800u;
    state.words[0u] = 0x01000001u;
    pal4::DynamicStripObjectConfiguration::UpdateFrameProperties(state, runtime);
    Check(runtime.calls.empty() && state.words[4u] != 0u,
          "hook mode preserves scale");
    state.words[0u] = 1u;
    state.words[14u] = 0u;
    pal4::DynamicStripObjectConfiguration::UpdateFrameProperties(state, runtime);
    Check(runtime.calls.empty(), "missing frame gate");

    state.words[14u] = 0x800u;
    Float(state, 28u, 10.0f);
    Float(state, 32u, 20.0f);
    Float(state, 36u, 30.0f);
    Float(state, 40u, 4.0f);
    Float(state, 44u, 5.0f);
    Float(state, 48u, 6.0f);
    pal4::DynamicStripObjectConfiguration::UpdateFrameProperties(state, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "identity", "scale", "rotate", "rotate", "rotate",
              "translate", "update"}, "native transform order");
    Check(state.words[4u] == 0u && state.words[6u] == 0u &&
              std::bit_cast<float>(state.words[5u]) == 2.0f,
          "negative scale components clamped in state");
    Check(runtime.scale.x == 0.0f && runtime.scale.y == 2.0f &&
              runtime.scale.z == 0.0f, "clamped scale forwarded");
    Check(runtime.axes.size() == 3u &&
              runtime.axes[0].x == 1.0f && runtime.axes[0].y == 0.0f &&
              runtime.axes[1].y == 1.0f && runtime.axes[2].z == 1.0f &&
              runtime.angles == std::vector<float>{10.0f, 20.0f, 30.0f},
          "XYZ axes and angles");
    Check(runtime.translation.x == 4.0f && runtime.translation.y == 5.0f &&
              runtime.translation.z == 6.0f, "translation forwarded");
}
