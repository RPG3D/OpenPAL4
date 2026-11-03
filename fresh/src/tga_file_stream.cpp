#include "pal4/tga_file_stream.h"

#include <array>
#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::string_view kSourceFile =
    "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\Common\\TgaLoader\\tga.c";
constexpr std::array<std::string_view, 8u> kMessages{
    "Success", "Error", "Out of memory", "Failed to open file",
    "Seek failed", "Read failed", "Write failed", "Unknown sub-format"};

}  // namespace

std::string_view TgaFileStream::ErrorMessage(const std::uint8_t code) {
    return kMessages[code < kMessages.size() ? code : 1u];
}

std::uint32_t TgaFileStream::Open(
    const std::uint32_t filename, const std::uint32_t mode,
    ITgaFileStreamRuntime& runtime) {
    const auto stream = runtime.Allocate(40u, 0u);
    if (stream == 0u) {
        runtime.LogError(kSourceFile, 36u, ErrorMessage(2u));
        return 0u;
    }
    runtime.WriteWord(stream + 4u, 0u);
    const auto handle = runtime.OpenHandle(filename, mode);
    if (handle == 0u) {
        runtime.NotifyError(stream, 3u);
        runtime.LogError(kSourceFile, 44u, ErrorMessage(3u));
        runtime.WriteWord(stream + 8u, 3u);
        (void)runtime.Free(stream);
        return 0u;
    }
    runtime.WriteWord(stream, handle);
    runtime.WriteWord(stream + 8u, 0u);
    return stream;
}

std::uint32_t TgaFileStream::Close(
    const std::uint32_t stream, ITgaFileStreamRuntime& runtime) {
    if (stream == 0u) return 0u;
    runtime.CloseHandle(runtime.ReadWord(stream));
    return runtime.Free(stream);
}

std::uint32_t TgaFileStream::Seek(
    const std::uint32_t stream, const std::int32_t offset,
    const std::int32_t origin, ITgaFileStreamRuntime& runtime) {
    const auto handle = runtime.ReadWord(stream);
    runtime.SeekHandle(handle, offset, origin);
    const auto position = runtime.Position(handle);
    runtime.WriteWord(stream + 4u, position);
    return position;
}

std::uint32_t TgaFileStream::Read(
    const std::uint32_t stream, const std::uint32_t buffer,
    const std::uint32_t size, const std::uint32_t count,
    ITgaFileStreamRuntime& runtime) {
    const auto handle = runtime.ReadWord(stream);
    const auto result = runtime.ReadHandle(buffer, size, count, handle);
    runtime.WriteWord(stream + 4u, runtime.Position(handle));
    return result;
}

void TgaFileStream::SwapBytes(
    const std::uint32_t data, const std::uint32_t limit,
    const std::uint32_t stride, ITgaFileStreamRuntime& runtime) {
    if (limit == 0u) return;
    if (stride == 0u)
        throw std::invalid_argument("TGA byte-swap stride cannot be zero");
    for (std::uint64_t offset = 0u; offset < limit; offset += stride) {
        const auto first = data + static_cast<std::uint32_t>(offset);
        const auto third = first + 2u;
        const auto old_first = runtime.ReadByte(first);
        const auto old_third = runtime.ReadByte(third);
        runtime.WriteByte(first, old_third);
        runtime.WriteByte(third, old_first);
    }
}

}  // namespace pal4
