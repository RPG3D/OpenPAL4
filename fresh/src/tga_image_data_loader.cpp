#include "pal4/tga_image_data_loader.h"

#include <stdexcept>

#include "pal4/tga_file_stream.h"

namespace pal4 {
namespace {

constexpr std::string_view kSourceFile =
    "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\Common\\TgaLoader\\tga.c";

std::uint16_t ReadHalfWord(const std::uint32_t address,
                           ITgaImageDataRuntime& runtime) {
    return static_cast<std::uint16_t>(runtime.ReadByte(address) |
        (static_cast<std::uint16_t>(runtime.ReadByte(address + 1u)) << 8u));
}

void ReportError(const std::uint32_t stream, const std::uint32_t code,
                 const std::uint32_t line, ITgaImageDataRuntime& runtime) {
    if (runtime.HasErrorCallback(stream)) runtime.NotifyError(stream, code);
    runtime.LogError(kSourceFile, line,
                     TgaFileStream::ErrorMessage(static_cast<std::uint8_t>(code)));
    runtime.WriteWord(stream + 8u, code);
}

}  // namespace

std::uint32_t TgaImageDataLoader::Load(
    const std::uint32_t stream, const std::uint32_t data_slot,
    const std::uint32_t flags, ITgaImageDataRuntime& runtime) {
    if (stream == 0u) return 0u;
    const auto depth = runtime.ReadByte(stream + 20u);
    const auto width = ReadHalfWord(stream + 18u, runtime);
    const auto size = static_cast<std::uint32_t>(width) * depth / 8u;
    if (size == 0u) return 0u;

    const auto offset = static_cast<std::uint32_t>(runtime.ReadByte(stream + 12u)) + 18u;
    if (runtime.ReadWord(stream + 4u) != offset) {
        runtime.Seek(stream, static_cast<std::int32_t>(offset), 0);
        if (runtime.ReadWord(stream + 4u) != offset) {
            ReportError(stream, 4u, 193u, runtime);
            return 0u;
        }
    }

    const auto data = runtime.Allocate(size, 0u);
    runtime.WriteWord(data_slot, data);
    if (runtime.Read(stream, data, size, 1u) != 1u) {
        ReportError(stream, 5u, 204u, runtime);
        return 0u;
    }
    if ((depth == 24u || depth == 32u) && (flags & 0x20u) != 0u)
        runtime.SwapBytes(data, size, depth >> 3u);

    if (depth == 15u || depth == 16u) {
        const auto resized = runtime.Reallocate(data, 1u, 0u);
        runtime.WriteWord(data_slot, resized);
        if (resized == 0u) {
            ReportError(stream, 2u, 216u, runtime);
            return 0u;
        }
        throw std::runtime_error(
            "TGA 15/16-bit conversion has no terminating branch at 0x5f9513");
    }
    runtime.WriteWord(stream + 8u, 0u);
    return 1u;
}

}  // namespace pal4
