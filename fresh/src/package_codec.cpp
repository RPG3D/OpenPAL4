#include "pal4/package_codec.h"

namespace pal4 {

std::int32_t PackageCodec::Run(PackageCodecState& state,
                               IPackageCodecRuntime& runtime) {
    if (state.mode == 1u) {
        state.result_bytes = state.source_bytes;
        state.succeeded = 1u;
        return 0;
    }
    if (state.mode != 2u)
        return static_cast<std::int32_t>(state.mode - 2u);

    const bool compress = state.compress != 0u;
    const auto result = compress
        ? runtime.Compress(state.source, state.source_bytes,
                           state.destination, state.result_bytes)
        : runtime.Decompress(state.source, state.source_bytes,
                             state.destination, state.result_bytes);
    if (result != 0) {
        state.succeeded = 0u;
        return runtime.ReportFailure(compress, result);
    }
    state.succeeded = 1u;
    return result;
}

}  // namespace pal4
