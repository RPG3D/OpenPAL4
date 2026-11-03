#pragma once

#include <cstdint>
#include <functional>
#include <array>
#include "pal4/rw_raster_copy.h"

namespace pal4 { class LegacyCrc32; }

namespace pal4 {

struct RwTextureDictionaryState final {
    std::uint32_t vtable{};
    std::uint32_t dictionary{};
    std::uint32_t enabled{};
};
static_assert(sizeof(RwTextureDictionaryState) == 12u);

class IRwTextureDictionaryRuntime {
public:
    virtual ~IRwTextureDictionaryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreateDictionary() = 0;
    virtual void ForEachTexture(
        std::uint32_t dictionary,
        const std::function<std::uint32_t(std::uint32_t)>& visit) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyDictionary(
        std::uint32_t dictionary) = 0;
    [[nodiscard]] virtual std::int32_t TextureReferences(
        std::uint32_t texture) = 0;
    virtual void SetTextureReferences(std::uint32_t texture,
                                      std::int32_t references) = 0;
};

class IRwTextureDictionaryContentRuntime : public IRwTextureDictionaryRuntime,
                                          public IRwRasterCopyRuntime {
public:
    virtual ~IRwTextureDictionaryContentRuntime() = default;
    virtual void CopyName(std::array<std::uint8_t, 256>& buffer,
                          const char* name, std::uint32_t count) = 0;
    virtual void NormalizeName(std::array<std::uint8_t, 256>& buffer) = 0;
    [[nodiscard]] virtual std::uint32_t NameLength(
        const std::array<std::uint8_t, 256>& buffer) = 0;
    [[nodiscard]] virtual std::uint32_t FindNamedTexture(
        std::uint32_t dictionary, const char* name) = 0;
    virtual void PrepareEmptyErrorString() = 0;
    virtual void SetCurrentDictionary(std::uint32_t dictionary) = 0;
    [[nodiscard]] virtual std::uint32_t ReadTexture(const char* name) = 0;
    virtual void SetTextureFilter(std::uint32_t texture,
                                  std::uint8_t value) = 0;
    virtual void SetTextureName(std::uint32_t texture,
                                const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t AddTextureToDictionary(
        std::uint32_t dictionary, std::uint32_t texture) = 0;
    virtual void RemoveTextureFromDictionary(std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::uint32_t CreateRaster(
        std::int32_t width, std::int32_t height, std::uint32_t depth,
        std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t CreateTexture(
        std::uint32_t raster) = 0;
};

class RwTextureDictionary final {
public:
    static constexpr std::uint32_t kVtableAddress = 0x84b940u;

    [[nodiscard]] static RwTextureDictionaryState& Construct(
        RwTextureDictionaryState& state) noexcept;
    static void Destruct(RwTextureDictionaryState& state) noexcept;
    [[nodiscard]] static bool Initialize(
        RwTextureDictionaryState& state,
        IRwTextureDictionaryRuntime& runtime);
    [[nodiscard]] static std::uint32_t DecrementTextureReferences(
        std::uint32_t texture, IRwTextureDictionaryRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        RwTextureDictionaryState& state,
        IRwTextureDictionaryRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindTextureByCrc(
        RwTextureDictionaryState& state, const char* name,
        IRwTextureDictionaryContentRuntime& runtime, LegacyCrc32& crc);
    [[nodiscard]] static std::uint32_t AddTexture(
        RwTextureDictionaryState& state, const char* name,
        IRwTextureDictionaryContentRuntime& runtime, LegacyCrc32& crc);
    [[nodiscard]] static bool RemoveTextureByCrc(
        RwTextureDictionaryState& state, const char* name,
        IRwTextureDictionaryContentRuntime& runtime, LegacyCrc32& crc);
    [[nodiscard]] static bool SetCurrent(
        RwTextureDictionaryState& state,
        IRwTextureDictionaryContentRuntime& runtime);
    [[nodiscard]] static std::uint32_t AddTextureFromPixels(
        RwTextureDictionaryState& state, const void* pixels,
        std::int32_t width, std::int32_t height, std::uint32_t depth,
        IRwTextureDictionaryContentRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateAndFillTexture(
        RwTextureDictionaryState& state, std::int32_t width,
        std::int32_t height, std::uint32_t depth, std::uint32_t color,
        IRwTextureDictionaryContentRuntime& runtime);
};

}  // namespace pal4
