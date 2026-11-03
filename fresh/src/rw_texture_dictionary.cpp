#include "pal4/rw_texture_dictionary.h"
#include "pal4/legacy_crc32.h"

#include <string>
#include <cstring>

namespace pal4 {

namespace {

std::string CrcTextureName(const char* name,
                           IRwTextureDictionaryContentRuntime& runtime,
                           LegacyCrc32& crc) {
    std::array<std::uint8_t, 256> buffer{};
    runtime.CopyName(buffer, name, 256);
    runtime.NormalizeName(buffer);
    const auto length = runtime.NameLength(buffer);
    return std::to_string(crc.ComputeAndStore(buffer.data(), length));
}

}  // namespace

std::uint32_t RwTextureDictionary::FindTextureByCrc(
    RwTextureDictionaryState& state, const char* name,
    IRwTextureDictionaryContentRuntime& runtime, LegacyCrc32& crc) {
    if (state.dictionary == 0) return 0;
    const auto decimal_name = CrcTextureName(name, runtime, crc);
    const auto texture = runtime.FindNamedTexture(
        state.dictionary, decimal_name.c_str());
    if (texture != 0)
        runtime.SetTextureReferences(
            texture, runtime.TextureReferences(texture) + 1);
    return texture;
}

std::uint32_t RwTextureDictionary::AddTexture(
    RwTextureDictionaryState& state, const char* name,
    IRwTextureDictionaryContentRuntime& runtime, LegacyCrc32& crc) {
    if (state.dictionary == 0 || name == nullptr || *name == '\0') return 0;
    const auto existing = FindTextureByCrc(state, name, runtime, crc);
    if (existing != 0) return existing;

    runtime.PrepareEmptyErrorString();
    runtime.SetCurrentDictionary(state.dictionary);
    const auto texture = runtime.ReadTexture(name);
    if (texture == 0) return 0;
    runtime.SetTextureFilter(texture, 2);

    const auto decimal_name = CrcTextureName(name, runtime, crc);
    runtime.SetTextureName(texture, decimal_name.c_str());
    return runtime.AddTextureToDictionary(state.dictionary, texture);
}

bool RwTextureDictionary::RemoveTextureByCrc(
    RwTextureDictionaryState& state, const char* name,
    IRwTextureDictionaryContentRuntime& runtime, LegacyCrc32& crc) {
    if (state.dictionary == 0) return false;
    const auto decimal_name = CrcTextureName(name, runtime, crc);
    const auto texture = runtime.FindNamedTexture(
        state.dictionary, decimal_name.c_str());
    if (texture == 0) return false;
    (void)DecrementTextureReferences(texture, runtime);
    runtime.RemoveTextureFromDictionary(texture);
    return true;
}

bool RwTextureDictionary::SetCurrent(
    RwTextureDictionaryState& state,
    IRwTextureDictionaryContentRuntime& runtime) {
    if (state.dictionary == 0) return false;
    runtime.SetCurrentDictionary(state.dictionary);
    return true;
}

std::uint32_t RwTextureDictionary::AddTextureFromPixels(
    RwTextureDictionaryState& state, const void* pixels,
    const std::int32_t width, const std::int32_t height,
    const std::uint32_t depth,
    IRwTextureDictionaryContentRuntime& runtime) {
    if (state.dictionary == 0) return 0;
    const auto raster = runtime.CreateRaster(width, height, depth, depth | 4u);
    if (raster == 0) return 0;
    const auto texture = runtime.CreateTexture(raster);
    if (texture == 0) return 0;
    runtime.SetTextureFilter(texture, 1);
    (void)RwRasterCopy::LockAndCopyData(
        texture, pixels, 0, 0, width, height, runtime);
    const auto sequence = ++state.enabled;
    const auto name = "texture-autoname(" + std::to_string(sequence) + ")";
    runtime.SetTextureName(texture, name.c_str());
    return runtime.AddTextureToDictionary(state.dictionary, texture);
}

std::uint32_t RwTextureDictionary::CreateAndFillTexture(
    RwTextureDictionaryState& state, const std::int32_t width,
    const std::int32_t height, const std::uint32_t depth,
    const std::uint32_t color,
    IRwTextureDictionaryContentRuntime& runtime) {
    if (state.dictionary == 0) return 0;
    const auto raster = runtime.CreateRaster(width, height, depth, depth | 4u);
    const auto texture = runtime.CreateTexture(raster);
    if (texture == 0) return 0;
    const auto texture_raster = runtime.TextureRaster(texture);
    runtime.SetTextureFilter(texture, 1);
    auto* locked = runtime.LockRaster(texture_raster, 0, 1);
    if (locked == nullptr) return 0;

    const auto bytes_per_pixel = runtime.RasterDepthBits(texture_raster) / 8;
    const auto stride = runtime.RasterStride(texture_raster);
    if (bytes_per_pixel > 0 && bytes_per_pixel <= 4) {
        for (std::int32_t row = 0; row < height; ++row) {
            auto* pixel = locked + row * stride;
            for (std::int32_t column = 0; column < width; ++column) {
                std::memcpy(pixel, &color,
                            static_cast<std::size_t>(bytes_per_pixel));
                pixel += bytes_per_pixel;
            }
        }
    }
    runtime.UnlockRaster(texture_raster);
    const auto sequence = ++state.enabled;
    const auto name = "texture-autoname(" + std::to_string(sequence) + ")";
    runtime.SetTextureName(texture, name.c_str());
    return runtime.AddTextureToDictionary(state.dictionary, texture);
}

}  // namespace pal4
