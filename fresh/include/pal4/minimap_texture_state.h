#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct MinimapTextureRecord {
    std::uint8_t enabled{};
    std::array<std::uint8_t, 3> reserved_01{};
    std::array<std::uint32_t, 4> region{};
    std::uint32_t grid{};
    std::uint32_t raster{};
    std::array<std::uint32_t, 2> reserved_1c{};
    std::uint32_t image{};
    std::uint32_t texture{};
};

static_assert(sizeof(MinimapTextureRecord) == 0x2cu);
static_assert(offsetof(MinimapTextureRecord, region) == 0x04u);
static_assert(offsetof(MinimapTextureRecord, grid) == 0x14u);
static_assert(offsetof(MinimapTextureRecord, raster) == 0x18u);
static_assert(offsetof(MinimapTextureRecord, image) == 0x24u);
static_assert(offsetof(MinimapTextureRecord, texture) == 0x28u);

class IMinimapRasterUpload {
public:
    virtual ~IMinimapRasterUpload() = default;
    virtual void SetRasterFromImage(std::uint32_t raster, std::uint32_t image) = 0;
};

class IMinimapTextureRuntime : public IMinimapRasterUpload {
public:
    virtual ~IMinimapTextureRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GridPixelBuffer(std::uint32_t grid) = 0;
    [[nodiscard]] virtual std::uint32_t GridField12(std::uint32_t grid) = 0;
    [[nodiscard]] virtual std::uint32_t GridStride(std::uint32_t grid) = 0;
    [[nodiscard]] virtual std::int32_t GridWidth(std::uint32_t grid) = 0;
    [[nodiscard]] virtual std::int32_t GridHeight(std::uint32_t grid) = 0;
    [[nodiscard]] virtual std::uint32_t ImageField(
        std::uint32_t image, std::uint32_t offset) = 0;
    virtual void ZeroBytes(std::uint32_t address, std::uint32_t size) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t CreateRaster(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t field12, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t CreateImage(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t field12) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateImagePixels(
        std::uint32_t image) = 0;
    [[nodiscard]] virtual std::uint32_t CreateTexture(
        std::uint32_t argument) = 0;
    virtual void SetTextureRaster(std::uint32_t texture,
                                  std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseImageData(
        std::uint32_t image) = 0;
    virtual void DestroyImage(std::uint32_t image) = 0;
    virtual void DestroyRaster(std::uint32_t raster) = 0;
    virtual void DestroyTexture(std::uint32_t texture) = 0;
};

class MinimapTextureState final {
public:
    MinimapTextureState() = default;
    explicit MinimapTextureState(MinimapTextureRecord record)
        : record_(record) {}

    [[nodiscard]] MinimapTextureState& Initialize(
        const std::array<std::uint32_t, 4>& region,
        std::uint32_t grid, bool mode, IMinimapTextureRuntime& runtime);
    [[nodiscard]] bool CreateFromGrid(
        const std::array<std::uint32_t, 4>& region,
        std::uint32_t grid, bool mode, IMinimapTextureRuntime& runtime);
    [[nodiscard]] bool UpdatePosition(std::int32_t x, std::int32_t y) noexcept;
    [[nodiscard]] std::uint32_t RefreshFromGrid(
        IMinimapTextureRuntime& runtime);
    void CommitRaster(IMinimapRasterUpload& runtime);
    void ResetThunk(IMinimapTextureRuntime& runtime);
    void Reset(IMinimapTextureRuntime& runtime);
    [[nodiscard]] const MinimapTextureRecord& Record() const noexcept {
        return record_;
    }

private:
    MinimapTextureRecord record_{};
};

}  // namespace pal4
