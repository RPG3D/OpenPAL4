#include "pal4/tga_color_map_loader.h"

#include <string_view>

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

}  // namespace

std::uint32_t TgaColorMapLoader::Load(const std::uint32_t stream,
                                      const std::uint32_t metadata,
                                      ITgaImageHeaderRuntime& runtime) {
    if (stream == 0u || runtime.ReadByte(stream + 12u) == 0u) return 0u;
    runtime.Seek(stream, 18, 0);
    if (runtime.ReadWord(stream + 4u) != 18u) {
        ReportError(stream, 4u, 159u, runtime);
        return 0u;
    }

    const auto size = runtime.ReadByte(stream + 12u);
    const auto color_map = runtime.Allocate(size, 0u);
    runtime.WriteWord(metadata, color_map);
    if (runtime.Read(stream, color_map, size, 1u) != 0u) {
        runtime.WriteWord(stream + 8u, 0u);
        return 0u;
    }
    // The native call at 0x5f9313 passes the metadata address, not color_map.
    runtime.Free(metadata);
    ReportError(stream, 5u, 170u, runtime);
    return 0u;
}

}  // namespace pal4
