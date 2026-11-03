#include "pal4/rw_texture_object.h"

#include <cstdio>

namespace pal4 {
namespace {

std::uint32_t ReleaseString(StringBufferState& text,
                            IRwTextureObjectRuntime& runtime) {
    std::uint32_t result = 0;
    if (text.data != 0) {
        const auto header = text.data - 1u;
        const auto references = runtime.ReadByte(header);
        if (references == 0u || references == 0xffu)
            result = runtime.Free(header);
        else {
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(references - 1u));
            result = header;
        }
    }
    text = {};
    return result;
}

}  // namespace

RwTextureObjectState& RwTextureObject::Construct(
    RwTextureObjectState& state, IRwTextureObjectRuntime& runtime) {
    state.active = 0;
    state.frame_rate = 30.0f;
    state.elapsed = 0.0f;
    state.clump = 0;
    state.name_flag = 0;
    state.name = {};
    runtime.InitializeEmpty(state.name);
    state.prefix_flag = 0;
    state.prefix = {};
    runtime.InitializeEmpty(state.prefix);
    state.texture_count = 0;
    state.texture_index = 0;
    state.textures = 0;
    state.vtable = kVtableAddress;
    return state;
}

std::uint32_t RwTextureObject::Destruct(
    RwTextureObjectState& state, IRwTextureObjectRuntime& runtime) {
    state.vtable = kVtableAddress;
    (void)ReleaseString(state.prefix, runtime);
    return ReleaseString(state.name, runtime);
}

bool RwTextureObject::Read(
    RwTextureObjectState& state, const char* source,
    const char* segment, const std::uint32_t clump_options,
    const char* frame_prefix, const float frame_duration,
    const std::uint32_t texture_count, IRwTextureObjectRuntime& runtime) {
    if (source == nullptr || segment == nullptr) return false;

    const std::string source_text(source);
    const std::string segment_text(segment);
    state.clump = runtime.LoadClump(
        source_text + segment_text + ".dff", source_text, clump_options);
    if (state.clump == 0) return false;

    runtime.AssignText(state.prefix, segment_text);
    runtime.AssignText(state.name, source_text);
    if (texture_count == 0 || frame_duration == 0.0f ||
        frame_prefix == nullptr)
        return false;

    state.texture_count = texture_count;
    state.elapsed = 0.0f;
    state.texture_index = 0;
    state.frame_rate = 1.0f / frame_duration;
    runtime.PrepareImageName(source_text);
    state.textures = runtime.AllocateTextureHandles(texture_count);
    if (state.textures == 0) return false;

    for (std::uint32_t index = 0; index < texture_count; ++index) {
        char suffix[32]{};
        std::snprintf(suffix, sizeof(suffix), "%02u.png", index + 1u);
        const std::string filename = std::string(frame_prefix) + suffix;
        runtime.StoreTexture(
            state.textures, index, runtime.ReadTexture(filename));
    }
    state.active = 1;
    return true;
}

bool RwTextureObject::Cleanup(
    RwTextureObjectState& state, const std::uint32_t dictionary,
    IRwTextureObjectRuntime& runtime) {
    if (state.active == 0) return false;
    state.active = 0;
    if (state.clump != 0) {
        if (runtime.WorldForClump(state.clump) != 0) {
            const auto world = runtime.WorldForClump(state.clump);
            runtime.RemoveClump(world, state.clump);
        }
        runtime.DestroyClump(state.clump);
        state.clump = 0;
    }

    const auto path = runtime.ReadText(state.name) +
                      runtime.ReadText(state.prefix) + ".dff";
    runtime.RemoveTextureByName(
        dictionary, runtime.NormalizeResourceName(path));
    runtime.AssignText(state.name, "");
    runtime.AssignText(state.prefix, "");

    if (state.textures != 0) {
        for (std::uint32_t index = 0; index < state.texture_count; ++index) {
            const auto texture = runtime.TextureAt(state.textures, index);
            if (texture != 0) runtime.DestroyTexture(texture);
        }
        (void)runtime.Free(state.textures);
        state.textures = 0;
    }
    state.frame_rate = 0.0f;
    state.elapsed = 0.0f;
    state.texture_count = 0;
    state.texture_index = 0;
    return true;
}

std::uint8_t RwTextureObject::Advance(
    RwTextureObjectState& state, const float delta,
    IRwTextureObjectRuntime& runtime) {
    if (state.active == 0) return 0;
    const double elapsed = static_cast<double>(state.elapsed) + delta;
    state.elapsed = static_cast<float>(elapsed);
    if (!(elapsed >= static_cast<double>(state.frame_rate))) return 0;
    state.elapsed = 0.0f;
    const auto next = state.texture_index + 1u;
    state.texture_index = next < state.texture_count ? next : 0u;
    return ApplyCurrentTexture(state, runtime);
}

std::uint8_t RwTextureObject::Render(
    const RwTextureObjectState& state, IRwTextureObjectRuntime& runtime) {
    if (state.active == 0) return 0;
    const auto world = runtime.WorldForClump(state.clump);
    if (world != 0) return static_cast<std::uint8_t>(world);
    return static_cast<std::uint8_t>(runtime.RenderClump(state.clump));
}

std::uint8_t RwTextureObject::ApplyCurrentTexture(
    const RwTextureObjectState& state, IRwTextureObjectRuntime& runtime) {
    if (state.active == 0 || state.textures == 0) return 0;
    const auto texture = runtime.TextureAt(
        state.textures, state.texture_index);
    return runtime.ApplyTexture(state.clump, texture);
}

}  // namespace pal4
