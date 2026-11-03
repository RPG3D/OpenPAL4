#pragma once

#include <cstdint>

namespace pal4 {

class IRwNormalMapRasterRuntime {
public:
    virtual ~IRwNormalMapRasterRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateRaster(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t depth, std::uint32_t format) = 0;
    [[nodiscard]] virtual std::uint32_t RasterLevelCount(
        std::uint32_t raster) = 0;
    virtual void LockRaster(std::uint32_t raster,
                            std::uint32_t level,
                            std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ResampleImage(
        std::uint32_t image, std::uint32_t width,
        std::uint32_t height) = 0;
    [[nodiscard]] virtual std::uint32_t CreateNormalMapImage(
        std::uint32_t image, bool use_alpha,
        float scale) = 0;
    virtual void SetRasterFromImage(std::uint32_t raster,
                                    std::uint32_t image) = 0;
    virtual void DestroyImage(std::uint32_t image) = 0;
    virtual void UnlockRaster(std::uint32_t raster) = 0;
};

class RwNormalMapRasterRuntimeStub : public IRwNormalMapRasterRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
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
    void DestroyImage(std::uint32_t) override {}
    void UnlockRaster(std::uint32_t) override {}
};

class RwNormalMapRasterBuilder final {
public:
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t image, std::uint32_t format,
        bool use_alpha, float scale,
        IRwNormalMapRasterRuntime& runtime);
};

}  // namespace pal4
