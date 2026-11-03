#include "pal4/dynamic_strip_object_config.h"

#include <bit>

namespace pal4 {

namespace {

template<class State> void WriteFloat(State& state,
                std::uint32_t offset, float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

template<class State> void WriteByte(State& state,
               std::uint32_t offset, std::uint8_t value) {
    auto& word = state.words[offset / 4u];
    const auto shift = 8u * (offset % 4u);
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

void SetEndpointFrame(std::uint32_t frame, float x,
                      IStripVisualFrameRuntime& runtime) {
    runtime.SetFrameIdentity(frame);
    runtime.ScaleFrame(frame, {1.0f, 1.0f, 1.0f}, 0);
    runtime.RotateFrame(frame, {1.0f, 0.0f, 0.0f}, 0.0f, 2);
    runtime.RotateFrame(frame, {0.0f, 1.0f, 0.0f}, 0.0f, 2);
    runtime.RotateFrame(frame, {0.0f, 0.0f, 1.0f}, 0.0f, 2);
    runtime.TranslateFrame(frame, {x, 0.0f, 0.0f}, 2);
    runtime.UpdateFrameObjects(frame);
}

}  // namespace

template<class State, class Runtime>
std::uint8_t Setup(State state, std::uint32_t texture, float width, float life, Runtime& runtime) {
    runtime.CleanupResources(state);
    if (texture == 0u || !(width > 0.0f) || !(life > 0.0f))
        return 0u;

    std::uint32_t texture_width{};
    std::uint32_t texture_height{};
    runtime.TextureSize(texture, texture_width, texture_height);
    state.words[16u] = texture;
    WriteFloat(state, 60u, life);
    WriteFloat(state, 68u, static_cast<float>(
        0.5 / static_cast<double>(texture_width)));
    WriteFloat(state, 72u, static_cast<float>(
        0.5 / static_cast<double>(texture_height)));
    runtime.AddTextureReference(texture);

    const float half_width = width * 0.5f;
    state.words[25u] = runtime.CreateFrame();
    SetEndpointFrame(state.words[25u], -half_width, runtime);
    state.words[26u] = runtime.CreateFrame();
    SetEndpointFrame(state.words[26u], half_width, runtime);
    state.words[14u] = runtime.CreateFrame();
    runtime.AddChildFrame(state.words[14u], state.words[25u]);
    runtime.AddChildFrame(state.words[14u], state.words[26u]);
    WriteByte(state, 0u, 1u);
    WriteByte(state, 2u, 1u);
    WriteByte(state, 3u, 0u);
    runtime.UpdateFrameProperties(state);
    return 1u;
}

std::uint8_t DynamicStripObjectConfiguration::VisualSetup(DynamicStripObjectState& s,
    std::uint32_t t,float w,float l,IDynamicStripVisualRuntime& r) {
    return Setup<DynamicStripObjectState&,IDynamicStripVisualRuntime>(s,t,w,l,r);
}
std::uint8_t DynamicStripObjectConfiguration::VisualSetup(StripVisualBlockView s,
    std::uint32_t t,float w,float l,IStripVisualBlockRuntime& r) {
    return Setup(s,t,w,l,r);
}
}  // namespace pal4
