#include "pal4/tga_header_loader.h"

#include "pal4/tga_file_stream.h"

namespace pal4 {
namespace {

constexpr std::string_view kSourceFile =
    "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\Common\\TgaLoader\\tga.c";

std::uint16_t ReadHalfWord(const std::uint32_t address,
                           ITgaHeaderRuntime& runtime) {
    return static_cast<std::uint16_t>(runtime.ReadByte(address) |
        (static_cast<std::uint16_t>(runtime.ReadByte(address + 1u)) << 8u));
}

void ReportError(const std::uint32_t stream, const std::uint32_t code,
                 const std::uint32_t line, const bool read_stream_code,
                 ITgaHeaderRuntime& runtime) {
    if (runtime.HasErrorCallback(stream)) runtime.NotifyError(stream, code);
    const auto message_code = read_stream_code
        ? runtime.ReadByte(stream + 8u) : static_cast<std::uint8_t>(code);
    runtime.LogError(kSourceFile, line,
        TgaFileStream::ErrorMessage(message_code));
}

}  // namespace

std::uint32_t TgaHeaderLoader::Load(const std::uint32_t stream,
                                    const std::uint32_t metadata,
                                    ITgaHeaderRuntime& runtime) {
    if (stream == 0u) return 0u;
    if (runtime.LoadImage(stream) != 0u) {
        ReportError(stream, runtime.ReadWord(stream + 8u), 45u, true, runtime);
        return 0u;
    }

    auto flags = runtime.ReadWord(metadata + 12u);
    if ((flags & 1u) != 0u && runtime.ReadByte(stream + 12u) != 0u) {
        if (runtime.LoadColorMap(stream, metadata) != 0u) {
            flags = runtime.ReadWord(metadata + 12u) & ~1u;
            runtime.WriteWord(metadata + 12u, flags);
            ReportError(stream, runtime.ReadWord(stream + 8u), 52u, true, runtime);
        }
    } else {
        flags &= ~1u;
        runtime.WriteWord(metadata + 12u, flags);
    }

    flags = runtime.ReadWord(metadata + 12u);
    if ((flags & 4u) == 0u) {
        runtime.WriteWord(stream + 8u, 0u);
        return 0u;
    }
    if (runtime.ReadByte(stream + 13u) == 1u) {
        if (runtime.LoadImageData(stream, metadata + 4u, flags) == 0u) {
            runtime.WriteWord(metadata + 12u,
                              runtime.ReadWord(metadata + 12u) & ~8u);
            ReportError(stream, runtime.ReadWord(stream + 8u), 62u, true, runtime);
            return 0u;
        }
        runtime.WriteWord(metadata + 12u,
                          runtime.ReadWord(metadata + 12u) | 8u);
    }

    const auto width = ReadHalfWord(stream + 26u, runtime);
    const auto height = ReadHalfWord(stream + 28u, runtime);
    const auto bits_per_pixel = runtime.ReadByte(stream + 30u);
    const auto size = static_cast<std::uint32_t>(
        static_cast<std::int32_t>(static_cast<std::uint32_t>(width) * height *
                                  bits_per_pixel) / 8);
    const auto pixels = runtime.Allocate(size, 0u);
    runtime.WriteWord(metadata + 8u, pixels);
    if (pixels == 0u) {
        runtime.WriteWord(metadata + 12u,
                          runtime.ReadWord(metadata + 12u) & ~4u);
        ReportError(stream, 2u, 72u, false, runtime);
        runtime.WriteWord(stream + 8u, 2u);
        return 0u;
    }

    const auto rows = runtime.LoadRows(stream, pixels, 0u, height,
                                       runtime.ReadWord(metadata + 12u));
    if (rows != height) {
        runtime.WriteWord(metadata + 12u,
                          runtime.ReadWord(metadata + 12u) & ~4u);
        ReportError(stream, runtime.ReadWord(stream + 8u), 78u, true, runtime);
    }
    runtime.WriteWord(stream + 8u, 0u);
    return 0u;
}

}  // namespace pal4
