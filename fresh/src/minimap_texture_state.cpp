#include "pal4/minimap_texture_state.h"

namespace pal4 {
namespace {

std::int32_t AddSigned32(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) +
        static_cast<std::uint32_t>(right));
}

}  // namespace

MinimapTextureState& MinimapTextureState::Initialize(
    const std::array<std::uint32_t, 4>& region,
    const std::uint32_t grid, const bool mode,
    IMinimapTextureRuntime& runtime) {
    record_.enabled = 0u;
    record_.raster = 0u;
    record_.image = 0u;
    record_.texture = 0u;
    (void)CreateFromGrid(region, grid, mode, runtime);
    return *this;
}

bool MinimapTextureState::CreateFromGrid(
    const std::array<std::uint32_t, 4>& region,
    const std::uint32_t grid, const bool mode,
    IMinimapTextureRuntime& runtime) {
    if (grid == 0u || runtime.GridPixelBuffer(grid) == 0u) {
        return false;
    }

    Reset(runtime);
    record_.region = region;
    record_.grid = grid;
    const std::uint32_t width = record_.region[2];
    const std::uint32_t height = record_.region[3];
    record_.raster = runtime.CreateRaster(
        width, height, runtime.GridField12(grid), mode ? 0x504u : 0x604u);
    if (record_.raster == 0u) {
        return false;
    }
    record_.image = runtime.CreateImage(width, height,
                                        runtime.GridField12(grid));
    if (record_.image == 0u) {
        Reset(runtime);
        return false;
    }
    record_.image = runtime.AllocateImagePixels(record_.image);
    if (record_.image == 0u) {
        Reset(runtime);
        return false;
    }
    record_.texture = runtime.CreateTexture(0u);
    runtime.SetTextureRaster(record_.texture, record_.raster);
    if (record_.texture == 0u) {
        Reset(runtime);
        return false;
    }
    return true;
}

bool MinimapTextureState::UpdatePosition(
    const std::int32_t x, const std::int32_t y) noexcept {
    const auto new_x = static_cast<std::uint32_t>(x);
    const auto new_y = static_cast<std::uint32_t>(y);
    if (record_.region[0] == new_x && record_.region[1] == new_y) {
        return false;
    }
    record_.region[0] = new_x;
    record_.region[1] = new_y;
    return true;
}

std::uint32_t MinimapTextureState::RefreshFromGrid(
    IMinimapTextureRuntime& runtime) {
    if (record_.image == 0u) {
        return 0u;
    }
    if (record_.grid == 0u) {
        return record_.image;
    }
    if (record_.raster == 0u) {
        return 0u;
    }
    if (runtime.GridPixelBuffer(record_.grid) == 0u) {
        return 0u;
    }
    const std::uint32_t depth = runtime.GridField12(record_.grid);
    if (runtime.ImageField(record_.image, 0x0cu) != depth) {
        return depth;
    }

    const auto x = static_cast<std::int32_t>(record_.region[0]);
    const auto y = static_cast<std::int32_t>(record_.region[1]);
    std::int32_t width = static_cast<std::int32_t>(record_.region[2]);
    const auto rows = static_cast<std::int32_t>(record_.region[3]);
    const std::uint32_t grid_pixels = runtime.GridPixelBuffer(record_.grid);
    const std::uint32_t image_stride =
        runtime.ImageField(record_.image, 0x10u);
    const std::uint32_t image_pixels =
        runtime.ImageField(record_.image, 0x14u);
    const std::uint32_t grid_stride = runtime.GridStride(record_.grid);
    const auto grid_height = runtime.GridHeight(record_.grid);
    const auto grid_width = runtime.GridWidth(record_.grid);
    if (x < 0) {
        width = AddSigned32(width, x);
    }
    if (width > grid_width) {
        width = grid_width;
    }
    if (AddSigned32(x, width) > grid_width) {
        width = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(grid_width) -
            static_cast<std::uint32_t>(x));
    }
    const std::uint32_t bytes_per_pixel = depth >> 3u;
    const std::uint32_t copy_bytes =
        static_cast<std::uint32_t>(width) * bytes_per_pixel;
    const std::int32_t x_byte_offset = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(x) * bytes_per_pixel);
    if (image_pixels == 0u) {
        return copy_bytes;
    }
    if (grid_pixels == 0u) {
        return 0u;
    }
    if (rows <= 0) {
        record_.enabled = 1u;
        return grid_pixels;
    }

    const std::uint32_t source_base = grid_pixels +
        static_cast<std::uint32_t>(y) * grid_stride +
        (x_byte_offset >= 0 ?
            static_cast<std::uint32_t>(x_byte_offset) : 0u);
    const std::uint32_t destination_offset = x_byte_offset < 0
        ? 0u - static_cast<std::uint32_t>(x_byte_offset) : 0u;
    for (std::int32_t row = 0; row < rows; ++row) {
        const std::uint32_t destination =
            image_pixels + static_cast<std::uint32_t>(row) * image_stride;
        runtime.ZeroBytes(destination, image_stride);
        const std::int32_t grid_row = AddSigned32(y, row);
        if (grid_row >= 0 && grid_row < grid_height) {
            runtime.CopyBytes(destination + destination_offset,
                              source_base +
                                  static_cast<std::uint32_t>(row) * grid_stride,
                              copy_bytes);
        }
    }
    record_.enabled = 1u;
    return 0u;
}

void MinimapTextureState::CommitRaster(IMinimapRasterUpload& runtime) {
    if (record_.image == 0u || record_.raster == 0u ||
        record_.enabled == 0u) {
        return;
    }
    record_.enabled = 0u;
    runtime.SetRasterFromImage(record_.raster, record_.image);
}

void MinimapTextureState::ResetThunk(IMinimapTextureRuntime& runtime) {
    Reset(runtime);
}

void MinimapTextureState::Reset(IMinimapTextureRuntime& runtime) {
    if (record_.image != 0u) {
        const std::uint32_t released = runtime.ReleaseImageData(record_.image);
        if (released != 0u) {
            runtime.DestroyImage(released);
        }
        record_.image = 0u;
    }
    if (record_.raster != 0u) {
        runtime.DestroyRaster(record_.raster);
        record_.raster = 0u;
    }
    if (record_.texture != 0u) {
        runtime.SetTextureRaster(record_.texture, 0u);
        runtime.DestroyTexture(record_.texture);
        record_.texture = 0u;
    }
}

}  // namespace pal4
