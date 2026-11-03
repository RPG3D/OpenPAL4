#include "pal4/clump_stream_loader.h"

namespace pal4 {

std::uint32_t ClumpStreamLoader::Load(
    const std::uint32_t source, const std::uint32_t image,
    IClumpStreamRuntime& runtime) {
    if (image != 0u) runtime.PrepareImage(image);
    const auto context = runtime.CreateContext(2u, 1u, source);
    if (context == 0u) return 0u;
    std::uint32_t clump = 0u;
    if (runtime.FindChunk(context, 16u, 0u, 0u)) {
        clump = runtime.ReadClump(context);
        runtime.LogLoadedClump(source, 241u);
    }
    runtime.CloseContext(context, 0u);
    return clump;
}

}  // namespace pal4
