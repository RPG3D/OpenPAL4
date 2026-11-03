#include "pal4/tga_image_header_reader.h"

#include "pal4/tga_file_stream.h"

namespace pal4 {
namespace {

constexpr std::string_view kSourceFile =
    "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\Common\\TgaLoader\\tga.c";

void ReportError(const std::uint32_t stream, const std::uint32_t code,
                 const std::uint32_t line, ITgaImageHeaderRuntime& runtime) {
    if (runtime.HasErrorCallback(stream)) runtime.NotifyError(stream, code);
    runtime.LogError(kSourceFile, line,
                     TgaFileStream::ErrorMessage(static_cast<std::uint8_t>(code)));
    runtime.WriteWord(stream + 8u, code);
}

std::uint16_t ReadHalfWord(const std::uint32_t address,
                           ITgaImageHeaderRuntime& runtime) {
    return static_cast<std::uint16_t>(runtime.ReadByte(address) |
        (static_cast<std::uint16_t>(runtime.ReadByte(address + 1u)) << 8u));
}

void WriteHalfWord(const std::uint32_t address, const std::uint16_t value,
                   ITgaImageHeaderRuntime& runtime) {
    runtime.WriteByte(address, static_cast<std::uint8_t>(value));
    runtime.WriteByte(address + 1u, static_cast<std::uint8_t>(value >> 8u));
}

bool SupportedDepth(const std::uint8_t depth) {
    return depth == 8u || depth == 15u || depth == 16u ||
           depth == 24u || depth == 32u;
}

}  // namespace

std::uint32_t TgaImageHeaderReader::Read(const std::uint32_t stream,
                                          ITgaImageHeaderRuntime& runtime) {
    if (stream == 0u) return 0u;
    runtime.Seek(stream, 0, 0);
    if (runtime.ReadWord(stream + 4u) != 0u) {
        ReportError(stream, 4u, 94u, runtime);
        return 0u;
    }

    const auto header = runtime.Allocate(18u, 0u);
    if (header == 0u) {
        ReportError(stream, 2u, 100u, runtime);
        return 0u;
    }
    for (std::uint32_t offset = 0u; offset < 18u; ++offset)
        runtime.WriteByte(header + offset, 0u);
    if (runtime.Read(stream, header, 18u, 1u) == 0u) {
        runtime.Free(header);
        ReportError(stream, 5u, 108u, runtime);
        return 0u;
    }

    runtime.WriteByte(stream + 12u, runtime.ReadByte(header));
    runtime.WriteByte(stream + 13u, runtime.ReadByte(header + 1u));
    runtime.WriteByte(stream + 14u, runtime.ReadByte(header + 2u));
    WriteHalfWord(stream + 16u, ReadHalfWord(header + 3u, runtime), runtime);
    WriteHalfWord(stream + 18u, ReadHalfWord(header + 5u, runtime), runtime);
    runtime.WriteByte(stream + 20u, runtime.ReadByte(header + 7u));
    for (std::uint32_t index = 0u; index < 4u; ++index)
        WriteHalfWord(stream + 22u + index * 2u,
                      ReadHalfWord(header + 8u + index * 2u, runtime), runtime);
    const auto depth = runtime.ReadByte(header + 16u);
    const auto descriptor = runtime.ReadByte(header + 17u);
    runtime.WriteByte(stream + 30u, depth);
    runtime.WriteByte(stream + 31u, descriptor & 0x0fu);
    runtime.WriteByte(stream + 32u, (descriptor & 0x10u) != 0u);
    runtime.WriteByte(stream + 33u, (descriptor & 0x20u) != 0u);

    if (runtime.ReadByte(stream + 13u) != 0u && depth != 8u) {
        ReportError(stream, 7u, 128u, runtime);
        runtime.Free(stream);
        runtime.Free(header);
        return 0u;
    }
    if (!SupportedDepth(depth)) {
        ReportError(stream, 7u, 140u, runtime);
        runtime.Free(stream);
        runtime.Free(header);
        return 0u;
    }
    runtime.Free(header);
    runtime.WriteWord(stream + 8u, 0u);
    return 0u;
}

}  // namespace pal4
