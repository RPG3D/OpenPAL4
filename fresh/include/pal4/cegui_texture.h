#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <vector>

namespace pal4 {

class CeguiTextureState final {
public:
    static constexpr std::uint32_t kBytes = 36u;
    static constexpr std::uint32_t kVtable = 0x842124u;

    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const noexcept;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t offset) const noexcept;
    void WriteDword(std::uint32_t offset, std::uint32_t value) noexcept;
    void WriteByte(std::uint32_t offset, std::uint8_t value) noexcept;
    void ReserveMaskBits(std::uint32_t bit_count);
    void AppendMaskBit(bool value);
    void InsertMaskBits(std::uint32_t index, std::uint32_t count, bool value);
    [[nodiscard]] bool MaskBit(std::uint32_t index) const;
    [[nodiscard]] std::uint32_t MaskBitCount() const noexcept;
    [[nodiscard]] std::uint32_t MaskWordCapacity() const noexcept;
    void ResetMask() noexcept;

private:
    std::array<std::uint8_t, kBytes> bytes_{};
    std::vector<std::uint32_t> mask_words_;
};

// Narrow image/readback boundary shared by native hit testing and the complete
// texture runtime. The native adapter does not implement unrelated file APIs.
class ICeguiTextureAlphaRuntime {
public:
    virtual ~ICeguiTextureAlphaRuntime() = default;
    virtual std::uint32_t DestroyPluginObject(std::uint32_t address) = 0;
    virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual std::uint16_t ReadWord(std::uint32_t address) = 0;
    virtual std::uint32_t CreateImage(std::uint32_t width,std::uint32_t height,std::uint32_t depth) = 0;
    virtual void AllocateImagePixels(std::uint32_t image) = 0;
    virtual void CopyRasterToImage(std::uint32_t image,std::uint32_t raster) = 0;
    virtual float AlphaFromArgb(std::uint32_t argb) = 0;
};

class ICeguiTextureRuntime : public ICeguiTextureAlphaRuntime {
public:
    virtual ~ICeguiTextureRuntime() = default;
    [[nodiscard]] virtual std::uint32_t DestroyPluginObject(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t BaseTextureVtable() = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint16_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeTextureDictionary() = 0;
    [[nodiscard]] virtual std::uint32_t BuildUtf8(
        std::uint32_t string_address) = 0;
    [[nodiscard]] virtual std::uint32_t LookupTexture(
        std::uint32_t dictionary, std::uint32_t utf8_address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateTexture(
        std::uint32_t dictionary, std::uint32_t width,
        std::uint32_t height, std::uint32_t color,
        std::uint32_t format) = 0;
    [[nodiscard]] virtual std::uint32_t LockAndCopyTexture(
        std::uint32_t dictionary, std::uint32_t texture,
        std::uint32_t source, std::uint32_t x, std::uint32_t y,
        std::uint32_t width, std::uint32_t height) = 0;
    [[nodiscard]] virtual std::uint32_t CreateImage(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t depth) = 0;
    virtual void AllocateImagePixels(std::uint32_t image) = 0;
    virtual void CopyRasterToImage(std::uint32_t image,
                                   std::uint32_t raster) = 0;
    virtual void ConstructColour(std::uint32_t& output,
                                 std::uint32_t argb) = 0;
    [[nodiscard]] virtual float AlphaFromArgb(std::uint32_t argb) = 0;
    virtual void ProcessImagePath(std::uint32_t image,
                                  std::uint32_t utf8_address) = 0;
};

class CeguiTextureRuntimeStub : public ICeguiTextureRuntime {
public:
    [[nodiscard]] std::uint32_t DestroyPluginObject(
        std::uint32_t address) override;
    [[nodiscard]] std::uint32_t Free(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t BaseTextureVtable() override;
    [[nodiscard]] std::uint32_t ReadDword(
        std::uint32_t address) override;
    [[nodiscard]] std::uint16_t ReadWord(
        std::uint32_t address) override;
    [[nodiscard]] std::uint32_t InitializeTextureDictionary() override;
    [[nodiscard]] std::uint32_t BuildUtf8(
        std::uint32_t string_address) override;
    [[nodiscard]] std::uint32_t LookupTexture(
        std::uint32_t dictionary, std::uint32_t utf8_address) override;
    [[nodiscard]] std::uint32_t CreateTexture(
        std::uint32_t dictionary, std::uint32_t width,
        std::uint32_t height, std::uint32_t color,
        std::uint32_t format) override;
    [[nodiscard]] std::uint32_t LockAndCopyTexture(
        std::uint32_t dictionary, std::uint32_t texture,
        std::uint32_t source, std::uint32_t x, std::uint32_t y,
        std::uint32_t width, std::uint32_t height) override;
    [[nodiscard]] std::uint32_t CreateImage(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t depth) override;
    void AllocateImagePixels(std::uint32_t image) override;
    void CopyRasterToImage(std::uint32_t image,
                           std::uint32_t raster) override;
    void ConstructColour(std::uint32_t& output,
                         std::uint32_t argb) override;
    [[nodiscard]] float AlphaFromArgb(std::uint32_t argb) override;
    void ProcessImagePath(std::uint32_t image,
                          std::uint32_t utf8_address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value);
    void WriteWord(std::uint32_t address, std::uint16_t value);

private:
    std::map<std::uint32_t, std::uint32_t> words_;
    std::map<std::uint32_t, std::uint16_t> half_words_;
};

class CeguiTexture final {
public:
    // 0x415cf0, 0x415d20, 0x415d40.
    [[nodiscard]] static CeguiTextureState& Construct(
        CeguiTextureState& state, std::uint32_t owner);
    [[nodiscard]] static CeguiTextureState& Destroy(
        CeguiTextureState& state, std::uint8_t flags,
        std::uint32_t object_address, ICeguiTextureRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        CeguiTextureState& state, ICeguiTextureRuntime& runtime);

    // 0x415d90, 0x415da0, 0x415db0.
    [[nodiscard]] static std::uint16_t NestedWord0C(
        const CeguiTextureState& state, ICeguiTextureAlphaRuntime& runtime);
    [[nodiscard]] static std::uint16_t NestedWord10(
        const CeguiTextureState& state, ICeguiTextureAlphaRuntime& runtime);
    [[nodiscard]] static std::uint32_t LoadByName(
        CeguiTextureState& state, std::uint32_t string_address,
        std::uint32_t unused, ICeguiTextureRuntime& runtime);
    // 0x415de0, 0x415e10, 0x415e40.
    [[nodiscard]] static std::uint32_t CreateWithDimensions(
        CeguiTextureState& state, std::uint32_t width,
        std::uint32_t height, std::uint32_t color,
        ICeguiTextureRuntime& runtime);
    [[nodiscard]] static std::uint32_t Update(
        const CeguiTextureState& state, std::uint32_t x,
        std::uint32_t y, std::uint32_t width,
        std::uint32_t height, std::uint32_t source,
        ICeguiTextureRuntime& runtime);
    [[nodiscard]] static std::uint32_t& ReadColour(
        CeguiTextureState& state, std::uint32_t& output,
        std::uint32_t x, std::uint32_t y,
        ICeguiTextureRuntime& runtime);
    // 0x415ea0: builds the alpha mask once, then tests a bit.
    [[nodiscard]] static bool HitTestAlpha(
        CeguiTextureState& state, std::uint32_t x,
        std::uint32_t y, ICeguiTextureAlphaRuntime& runtime);
    // 0x4160f0: exports a temporary raster image through a named path.
    [[nodiscard]] static std::uint32_t ProcessImageByName(
        const CeguiTextureState& state, std::uint32_t string_address,
        ICeguiTextureRuntime& runtime);
};

}  // namespace pal4
