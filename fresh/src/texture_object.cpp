#include "pal4/texture_object.h"

namespace pal4 {

std::uint32_t TextureObject::Destroy(
    TextureObjectState& state, ITextureObjectRuntime& runtime) {
    if (state.first_raster != 0u) {
        runtime.DestroyRaster(state.first_raster);
        state.first_raster = 0u;
    }
    if (state.second_raster != 0u) {
        runtime.DestroyRaster(state.second_raster);
        state.second_raster = 0u;
    }
    std::uint32_t result = state.texture;
    if (state.texture != 0u) {
        runtime.SetTextureRaster(state.texture, 0u);
        result = runtime.DestroyTexture(state.texture);
        state.texture = 0u;
    }
    state.context = 0u;
    return result;
}

bool TextureObject::Initialize(
    TextureObjectState& state, const std::uint32_t context,
    const std::uint32_t width, const std::uint32_t height,
    ITextureObjectRuntime& runtime) {
    (void)Destroy(state, runtime);
    state.context = context;
    state.first_raster = runtime.CreateRaster(width, height, 0u, 5u);
    if (state.first_raster == 0u) return false;
    state.second_raster = runtime.CreateRaster(width, height, 0u, 1u);
    if (state.second_raster == 0u) {
        runtime.DestroyRaster(state.first_raster);
        return false;
    }
    state.texture = runtime.CreateTexture(state.first_raster);
    runtime.SetTextureFilter(state.texture, 2u);
    return true;
}

std::uint32_t TextureObject::ApplyAndRevertProperties(
    TextureObjectState& state, const float value,
    const std::uint32_t callback, ITextureObjectRuntime& runtime) {
    const auto original_first = runtime.ReadWord(state.context + 96u);
    const auto original_second = runtime.ReadWord(state.context + 100u);
    runtime.WriteWord(state.context + 96u, state.first_raster);
    runtime.WriteWord(state.context + 100u, state.second_raster);
    try {
        runtime.InvokeCallback(callback, static_cast<double>(value),
                               state.context);
    } catch (...) {
        runtime.WriteWord(state.context + 96u, original_first);
        runtime.WriteWord(state.context + 100u, original_second);
        throw;
    }
    runtime.WriteWord(state.context + 96u, original_first);
    runtime.WriteWord(state.context + 100u, original_second);
    return state.context;
}

}  // namespace pal4
