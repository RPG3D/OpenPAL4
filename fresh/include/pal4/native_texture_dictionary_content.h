#pragma once
#include "pal4/texture_dictionary_native.h"
#include "pal4/rw_raster_copy_native.h"

namespace pal4 {
// The reader runs synchronously on the SDK thread. Archive/path scopes remain
// with the caller; no second cache or image decoder is introduced here.
class NativeTextureDictionaryContent final : public IRwTextureDictionaryContentRuntime {
public:
    explicit NativeTextureDictionaryContent(std::function<std::uint32_t(const char*)> reader);
    std::uint32_t CreateDictionary() override;
    void ForEachTexture(std::uint32_t,const std::function<std::uint32_t(std::uint32_t)>&) override;
    std::uint32_t DestroyDictionary(std::uint32_t) override;
    std::int32_t TextureReferences(std::uint32_t) override;
    void SetTextureReferences(std::uint32_t,std::int32_t) override;
    void CopyName(std::array<std::uint8_t,256>&,const char*,std::uint32_t) override;
    void NormalizeName(std::array<std::uint8_t,256>&) override;
    std::uint32_t NameLength(const std::array<std::uint8_t,256>&) override;
    std::uint32_t FindNamedTexture(std::uint32_t,const char*) override;
    void PrepareEmptyErrorString() override;
    void SetCurrentDictionary(std::uint32_t) override;
    std::uint32_t ReadTexture(const char*) override;
    void SetTextureFilter(std::uint32_t,std::uint8_t) override;
    void SetTextureName(std::uint32_t,const char*) override;
    std::uint32_t AddTextureToDictionary(std::uint32_t,std::uint32_t) override;
    void RemoveTextureFromDictionary(std::uint32_t) override;
    std::uint32_t CreateRaster(std::int32_t,std::int32_t,std::uint32_t,std::uint32_t) override;
    std::uint32_t CreateTexture(std::uint32_t) override;
    std::uint32_t TextureRaster(std::uint32_t) override;
    std::int32_t RasterWidth(std::uint32_t) override;
    std::int32_t RasterHeight(std::uint32_t) override;
    std::int32_t RasterDepthBits(std::uint32_t) override;
    std::int32_t RasterStride(std::uint32_t) override;
    std::uint8_t* LockRaster(std::uint32_t,std::int32_t,std::int32_t) override;
    void UnlockRaster(std::uint32_t) override;
private:
    NativeTextureDictionaryRuntime dictionaries_;
    NativeRwRasterCopyRuntime rasters_;
    std::function<std::uint32_t(const char*)> reader_;
};
}
