#pragma once

#include "pal4/rw_normal_map_raster_builder.h"

#include <cstdint>

namespace pal4 {

class IRwTextureImageFactoryRuntime : public IRwNormalMapRasterRuntime {
public:
    virtual ~IRwTextureImageFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateImage(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t depth) = 0;
    virtual void AllocatePixels(std::uint32_t image) = 0;
    virtual void SetImageFromRaster(std::uint32_t image,
                                    std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::uint32_t CreateTexture(
        std::uint32_t raster) = 0;
};

class RwTextureImageFactoryRuntimeStub final
    : public IRwTextureImageFactoryRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    std::uint8_t ReadByte(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    std::uint32_t CreateImage(std::uint32_t, std::uint32_t,
                              std::uint32_t) override { return 0; }
    void AllocatePixels(std::uint32_t) override {}
    void SetImageFromRaster(std::uint32_t, std::uint32_t) override {}
    std::uint32_t CreateRaster(
        std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t) override { return 0; }
    std::uint32_t RasterLevelCount(std::uint32_t) override { return 0; }
    void LockRaster(std::uint32_t, std::uint32_t,
                    std::uint32_t) override {}
    std::uint32_t ResampleImage(
        std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0; }
    std::uint32_t CreateNormalMapImage(
        std::uint32_t, bool, float) override { return 0; }
    void SetRasterFromImage(std::uint32_t,
                            std::uint32_t) override {}
    void UnlockRaster(std::uint32_t) override {}
    std::uint32_t CreateTexture(std::uint32_t) override { return 0; }
    void DestroyImage(std::uint32_t) override {}
};

class RwTextureImageFactory final {
public:
    [[nodiscard]] static std::uint32_t CreateFromTexture(
        std::uint32_t texture, float scale,
        IRwTextureImageFactoryRuntime& runtime);
};

}  // namespace pal4
