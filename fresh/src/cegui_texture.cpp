#include "pal4/cegui_texture.h"

#include "pal4/cegui_bit_array.h"

#include <limits>
#include <stdexcept>

namespace pal4 {

std::uint32_t CeguiTextureState::ReadDword(
    const std::uint32_t offset) const noexcept {
    std::uint32_t value = 0u;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        value |= static_cast<std::uint32_t>(bytes_[offset + index])
                 << (8u * index);
    }
    return value;
}
std::uint8_t CeguiTextureState::ReadByte(
    const std::uint32_t offset) const noexcept { return bytes_[offset]; }
void CeguiTextureState::WriteDword(
    const std::uint32_t offset, const std::uint32_t value) noexcept {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        bytes_[offset + index] =
            static_cast<std::uint8_t>(value >> (8u * index));
    }
}
void CeguiTextureState::WriteByte(
    const std::uint32_t offset, const std::uint8_t value) noexcept {
    bytes_[offset] = value;
}
void CeguiTextureState::ReserveMaskBits(const std::uint32_t bit_count) {
    mask_words_.reserve((static_cast<std::size_t>(bit_count) + 31u) / 32u);
}
void CeguiTextureState::AppendMaskBit(const bool value) {
    InsertMaskBits(MaskBitCount(), 1u, value);
}
void CeguiTextureState::InsertMaskBits(
    const std::uint32_t index, const std::uint32_t count, const bool value) {
    const auto old_count = MaskBitCount();
    if (index > old_count) {
        throw std::out_of_range("CEGUI texture mask insertion index");
    }
    if (count == 0u) {
        return;
    }
    const auto new_count = static_cast<std::uint64_t>(old_count) + count;
    if (new_count > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("CEGUI texture mask bit count overflow");
    }
    mask_words_.resize((static_cast<std::size_t>(new_count) + 31u) / 32u);
    const auto set_bit = [this](const std::uint32_t bit, const bool set) {
        auto& word = mask_words_[bit >> 5u];
        const auto mask = 1u << (bit & 31u);
        word = set ? word | mask : word & ~mask;
    };
    for (auto source = old_count; source > index; --source) {
        const auto bit = source - 1u;
        set_bit(bit + count, MaskBit(bit));
    }
    for (auto bit = index; bit < index + count; ++bit) {
        set_bit(bit, value);
    }
    WriteDword(16u, static_cast<std::uint32_t>(new_count));
}
bool CeguiTextureState::MaskBit(const std::uint32_t index) const {
    if (index >= MaskBitCount()) {
        throw std::out_of_range("CEGUI texture mask index");
    }
    return (mask_words_.at(index >> 5u) & (1u << (index & 31u))) != 0u;
}
std::uint32_t CeguiTextureState::MaskBitCount() const noexcept {
    return ReadDword(16u);
}
std::uint32_t CeguiTextureState::MaskWordCapacity() const noexcept {
    return static_cast<std::uint32_t>(mask_words_.capacity());
}
void CeguiTextureState::ResetMask() noexcept {
    std::vector<std::uint32_t>().swap(mask_words_);
    WriteDword(16u, 0u);
}

std::uint32_t CeguiTextureRuntimeStub::DestroyPluginObject(
    std::uint32_t /*address*/) { return 0u; }
std::uint32_t CeguiTextureRuntimeStub::Free(
    std::uint32_t /*address*/) { return 0u; }
std::uint32_t CeguiTextureRuntimeStub::BaseTextureVtable() { return 0u; }
std::uint32_t CeguiTextureRuntimeStub::ReadDword(
    const std::uint32_t address) { return words_.at(address); }
std::uint16_t CeguiTextureRuntimeStub::ReadWord(
    const std::uint32_t address) { return half_words_.at(address); }
std::uint32_t CeguiTextureRuntimeStub::InitializeTextureDictionary() {
    return 0u;
}
std::uint32_t CeguiTextureRuntimeStub::BuildUtf8(
    std::uint32_t /*string_address*/) { return 0u; }
std::uint32_t CeguiTextureRuntimeStub::LookupTexture(
    std::uint32_t /*dictionary*/, std::uint32_t /*utf8_address*/) {
    return 0u;
}
std::uint32_t CeguiTextureRuntimeStub::CreateTexture(
    std::uint32_t /*dictionary*/, std::uint32_t /*width*/,
    std::uint32_t /*height*/, std::uint32_t /*color*/,
    std::uint32_t /*format*/) { return 0u; }
std::uint32_t CeguiTextureRuntimeStub::LockAndCopyTexture(
    std::uint32_t /*dictionary*/, std::uint32_t /*texture*/,
    std::uint32_t /*source*/, std::uint32_t /*x*/,
    std::uint32_t /*y*/, std::uint32_t /*width*/,
    std::uint32_t /*height*/) { return 0u; }
std::uint32_t CeguiTextureRuntimeStub::CreateImage(
    std::uint32_t /*width*/, std::uint32_t /*height*/,
    std::uint32_t /*depth*/) { return 0u; }
void CeguiTextureRuntimeStub::AllocateImagePixels(
    std::uint32_t /*image*/) {}
void CeguiTextureRuntimeStub::CopyRasterToImage(
    std::uint32_t /*image*/, std::uint32_t /*raster*/) {}
void CeguiTextureRuntimeStub::ConstructColour(
    std::uint32_t& output, const std::uint32_t argb) { output = argb; }
float CeguiTextureRuntimeStub::AlphaFromArgb(const std::uint32_t argb) {
    return static_cast<float>((argb >> 24u) & 0xffu) / 255.0f;
}
void CeguiTextureRuntimeStub::ProcessImagePath(
    std::uint32_t /*image*/, std::uint32_t /*utf8_address*/) {}
void CeguiTextureRuntimeStub::WriteDword(
    const std::uint32_t address, const std::uint32_t value) {
    words_[address] = value;
}
void CeguiTextureRuntimeStub::WriteWord(
    const std::uint32_t address, const std::uint16_t value) {
    half_words_[address] = value;
}

CeguiTextureState& CeguiTexture::Construct(
    CeguiTextureState& state, const std::uint32_t owner) {
    state.ResetMask();
    state.WriteDword(4u, owner);
    state.WriteDword(8u, 0u);
    state.WriteDword(12u, 0u);
    state.WriteDword(16u, 0u);
    state.WriteByte(20u, static_cast<std::uint8_t>(owner));
    state.WriteDword(24u, 0u);
    state.WriteDword(28u, 0u);
    state.WriteDword(32u, 0u);
    state.WriteDword(0u, CeguiTextureState::kVtable);
    return state;
}

CeguiTextureState& CeguiTexture::Destroy(
    CeguiTextureState& state, const std::uint8_t flags,
    const std::uint32_t object_address, ICeguiTextureRuntime& runtime) {
    static_cast<void>(Cleanup(state, runtime));
    if ((flags & 1u) != 0u) {
        static_cast<void>(runtime.Free(object_address));
    }
    return state;
}

std::uint32_t CeguiTexture::Cleanup(
    CeguiTextureState& state, ICeguiTextureRuntime& runtime) {
    const auto plugin = state.ReadDword(12u);
    state.WriteDword(0u, CeguiTextureState::kVtable);
    if (plugin != 0u) {
        static_cast<void>(runtime.DestroyPluginObject(plugin));
        state.WriteDword(12u, 0u);
    }
    state.ResetMask();
    const auto result = runtime.Free(state.ReadDword(24u));
    state.WriteDword(24u, 0u);
    state.WriteDword(28u, 0u);
    state.WriteDword(32u, 0u);
    state.WriteDword(0u, runtime.BaseTextureVtable());
    return result;
}

std::uint16_t CeguiTexture::NestedWord0C(
    const CeguiTextureState& state, ICeguiTextureAlphaRuntime& runtime) {
    const auto object = state.ReadDword(8u);
    return object == 0u ? 0u :
        runtime.ReadWord(runtime.ReadDword(object) + 12u);
}

std::uint16_t CeguiTexture::NestedWord10(
    const CeguiTextureState& state, ICeguiTextureAlphaRuntime& runtime) {
    const auto object = state.ReadDword(8u);
    return object == 0u ? 0u :
        runtime.ReadWord(runtime.ReadDword(object) + 16u);
}

std::uint32_t CeguiTexture::LoadByName(
    CeguiTextureState& state, const std::uint32_t string_address,
    const std::uint32_t unused, ICeguiTextureRuntime& runtime) {
    static_cast<void>(unused);
    const auto dictionary = runtime.InitializeTextureDictionary();
    const auto utf8 = runtime.BuildUtf8(string_address);
    const auto texture = runtime.LookupTexture(dictionary, utf8);
    state.WriteDword(8u, texture);
    return texture;
}

std::uint32_t CeguiTexture::CreateWithDimensions(
    CeguiTextureState& state, const std::uint32_t width,
    const std::uint32_t height, const std::uint32_t color,
    ICeguiTextureRuntime& runtime) {
    const auto dictionary = runtime.InitializeTextureDictionary();
    const auto texture = runtime.CreateTexture(dictionary, width, height,
                                               color, 1280u);
    state.WriteDword(8u, texture);
    return texture;
}

std::uint32_t CeguiTexture::Update(
    const CeguiTextureState& state, const std::uint32_t x,
    const std::uint32_t y, const std::uint32_t width,
    const std::uint32_t height, const std::uint32_t source,
    ICeguiTextureRuntime& runtime) {
    const auto dictionary = runtime.InitializeTextureDictionary();
    return runtime.LockAndCopyTexture(dictionary, state.ReadDword(8u),
                                      source, x, y, width, height);
}

std::uint32_t& CeguiTexture::ReadColour(
    CeguiTextureState& state, std::uint32_t& output,
    const std::uint32_t x, const std::uint32_t y,
    ICeguiTextureRuntime& runtime) {
    auto image = state.ReadDword(12u);
    if (image == 0u) {
        const auto raster = runtime.ReadDword(state.ReadDword(8u));
        const auto height = runtime.ReadDword(raster + 16u);
        const auto width = runtime.ReadDword(raster + 12u);
        image = runtime.CreateImage(width, height, 32u);
        state.WriteDword(12u, image);
        runtime.AllocateImagePixels(image);
        runtime.CopyRasterToImage(image, raster);
    }
    const auto stride = runtime.ReadDword(image + 16u);
    const auto pixels = runtime.ReadDword(image + 20u);
    const auto argb = runtime.ReadDword(pixels + y * stride + 4u * x);
    runtime.ConstructColour(output, argb);
    return output;
}

bool CeguiTexture::HitTestAlpha(
    CeguiTextureState& state, const std::uint32_t x,
    const std::uint32_t y, ICeguiTextureAlphaRuntime& runtime) {
    if (state.MaskBitCount() == 0u) {
        const auto raster = runtime.ReadDword(state.ReadDword(8u));
        const auto height = runtime.ReadDword(raster + 16u);
        const auto width = runtime.ReadDword(raster + 12u);
        const auto image = runtime.CreateImage(width, height, 32u);
        try {
            runtime.AllocateImagePixels(image);
            runtime.CopyRasterToImage(image, raster);
            const auto stride = runtime.ReadDword(image + 16u);
            const auto pixels = runtime.ReadDword(image + 20u);
            const auto mask_width = NestedWord0C(state, runtime);
            const auto mask_height = NestedWord10(state, runtime);
            const auto count = static_cast<std::uint64_t>(mask_width) *
                               mask_height;
            if (count > std::numeric_limits<std::uint32_t>::max()) {
                throw std::length_error("CEGUI texture mask dimensions overflow");
            }
            state.ReserveMaskBits(static_cast<std::uint32_t>(count));
            for (std::uint32_t row = 0u; row < mask_height; ++row) {
                for (std::uint32_t column = 0u; column < mask_width;
                     ++column) {
                    const auto argb = runtime.ReadDword(
                        pixels + row * stride + 4u * column);
                    state.AppendMaskBit(runtime.AlphaFromArgb(argb) < 0.1f);
                }
            }
        } catch (...) {
            static_cast<void>(runtime.DestroyPluginObject(image));
            throw;
        }
        static_cast<void>(runtime.DestroyPluginObject(image));
    }
    const auto width = NestedWord0C(state, runtime);
    const auto index = x + y * static_cast<std::uint32_t>(width);
    return state.MaskBit(index);
}

std::uint32_t CeguiTexture::ProcessImageByName(
    const CeguiTextureState& state, const std::uint32_t string_address,
    ICeguiTextureRuntime& runtime) {
    const auto raster = runtime.ReadDword(state.ReadDword(8u));
    const auto height = runtime.ReadDword(raster + 16u);
    const auto width = runtime.ReadDword(raster + 12u);
    const auto image = runtime.CreateImage(width, height, 32u);
    runtime.AllocateImagePixels(image);
    runtime.CopyRasterToImage(image, raster);
    const auto utf8 = runtime.BuildUtf8(string_address);
    runtime.ProcessImagePath(image, utf8);
    return runtime.DestroyPluginObject(image);
}

}  // namespace pal4
