#include "pal4/tga_image_rows_loader.h"

#include <stdexcept>

#include "pal4/tga_file_stream.h"

namespace pal4 {
namespace {

constexpr std::string_view kSourceFile =
    "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\Common\\TgaLoader\\tga.c";

std::uint16_t ReadHalfWord(const std::uint32_t address,
                           ITgaImageRowsRuntime& runtime) {
    return static_cast<std::uint16_t>(runtime.ReadByte(address) |
        (static_cast<std::uint16_t>(runtime.ReadByte(address + 1u)) << 8u));
}

void ReportError(const std::uint32_t stream, const std::uint32_t code,
                 const std::uint32_t line, ITgaImageRowsRuntime& runtime) {
    if (runtime.HasErrorCallback(stream)) runtime.NotifyError(stream, code);
    runtime.LogError(kSourceFile, line,
                     TgaFileStream::ErrorMessage(static_cast<std::uint8_t>(code)));
    runtime.WriteWord(stream + 8u, code);
}

}  // namespace

std::uint32_t TgaImageRowsLoader::Load(
    const std::uint32_t stream, const std::uint32_t pixels,
    const std::uint32_t first_row, const std::uint32_t row_count,
    const std::uint32_t flags, ITgaImageRowsRuntime& runtime) {
    if (stream == 0u || pixels == 0u) return 0u;
    const auto width = ReadHalfWord(stream + 26u, runtime);
    const auto depth = runtime.ReadByte(stream + 30u);
    const auto row_stride = static_cast<std::uint32_t>(width) * depth / 8u;
    const auto start_offset = first_row * row_stride;
    const auto color_map_size = static_cast<std::uint32_t>(
        ReadHalfWord(stream + 18u, runtime)) *
        runtime.ReadByte(stream + 20u) / 8u;
    const auto file_offset = 18u + runtime.ReadByte(stream + 12u) +
                             color_map_size + start_offset;
    if (runtime.ReadWord(stream + 4u) != file_offset) {
        runtime.Seek(stream, static_cast<std::int32_t>(file_offset), 0);
        if (runtime.ReadWord(stream + 4u) != file_offset) {
            ReportError(stream, 4u, 296u, runtime);
            return 0u;
        }
    }

    std::uint32_t rows = 0u;
    const auto image_type = runtime.ReadByte(stream + 14u);
    if (image_type > 8u && image_type < 12u) {
        auto row_address = pixels + start_offset;
        do {
            if (runtime.LoadRleRow(stream, row_address) != 0u) break;
            ++rows;
            row_address += row_stride;
        } while (rows <= row_count);
        runtime.WriteByte(stream + 14u,
                          static_cast<std::uint8_t>(image_type - 8u));
    } else {
        rows = runtime.Read(stream, pixels, row_stride, row_count);
    }

    if (rows != row_count) {
        ReportError(stream, 5u, 310u, runtime);
        return rows;
    }
    if ((depth == 24u || depth == 32u) && (flags & 0x20u) != 0u)
        runtime.SwapBytes(pixels + start_offset, rows * row_stride,
                          depth >> 3u);
    if (depth == 15u || depth == 16u) {
        const auto resized = runtime.Reallocate(pixels, rows + (rows >> 1u), 0u);
        if (resized == 0u) {
            ReportError(stream, 2u, 323u, runtime);
            return 0u;
        }
        throw std::runtime_error(
            "TGA 15/16-bit row conversion has no terminating branch at 0x5f98bf");
    }
    runtime.WriteWord(stream + 8u, 0u);
    return rows;
}

}  // namespace pal4
