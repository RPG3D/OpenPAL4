#include "pal4/package_codec.h"

#include "minilzo.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class MiniLzoRuntime final : public pal4::IPackageCodecRuntime {
public:
    MiniLzoRuntime() : work_memory(LZO1X_1_MEM_COMPRESS),
                       source(1024u), compressed(1024u + 1024u / 16u + 67u),
                       decoded(1024u) {
        for (std::size_t i = 0; i < source.size(); ++i)
            source[i] = static_cast<unsigned char>((i / 32u) & 0x0fu);
    }

    std::int32_t Compress(std::uint32_t source_id, std::uint32_t bytes,
                          std::uint32_t destination_id,
                          std::uint32_t& result_bytes) override {
        Check(source_id == 1u && destination_id == 2u &&
              bytes == source.size(), "compression address/length mapping");
        lzo_uint length = static_cast<lzo_uint>(compressed.size());
        const int result = lzo1x_1_compress(source.data(), bytes,
                                            compressed.data(), &length,
                                            work_memory.data());
        result_bytes = static_cast<std::uint32_t>(length);
        return result;
    }
    std::int32_t Decompress(std::uint32_t source_id, std::uint32_t bytes,
                            std::uint32_t destination_id,
                            std::uint32_t& result_bytes) override {
        Check(source_id == 2u && destination_id == 3u &&
              bytes <= compressed.size(), "decompression address/length mapping");
        lzo_uint length = static_cast<lzo_uint>(decoded.size());
        const int result = lzo1x_decompress_safe(
            compressed.data(), bytes, decoded.data(), &length, nullptr);
        result_bytes = static_cast<std::uint32_t>(length);
        return result;
    }
    std::int32_t ReportFailure(bool compressing,
                                std::int32_t result) override {
        ++reports;
        last_compressing = compressing;
        last_error = result;
        return 73;
    }

    std::vector<unsigned char> work_memory;
    std::vector<unsigned char> source;
    std::vector<unsigned char> compressed;
    std::vector<unsigned char> decoded;
    int reports = 0;
    bool last_compressing = false;
    std::int32_t last_error = 0;
};

}  // namespace

int main() {
    Check(lzo_init() == LZO_E_OK, "external MiniLZO initialization");
    MiniLzoRuntime runtime;
    pal4::PackageCodecState state;
    state.mode = 2u;
    state.compress = 1u;
    state.source = 1u;
    state.destination = 2u;
    state.source_bytes = static_cast<std::uint32_t>(runtime.source.size());
    Check(pal4::PackageCodec::Run(state, runtime) == 0 &&
          state.succeeded == 1u && state.result_bytes > 0u &&
          state.result_bytes < state.source_bytes,
          "game wrapper compresses through public LZO1X API");
    const auto compressed_bytes = state.result_bytes;

    state.compress = 0u;
    state.source = 2u;
    state.destination = 3u;
    state.source_bytes = compressed_bytes;
    Check(pal4::PackageCodec::Run(state, runtime) == 0 &&
          state.succeeded == 1u && state.result_bytes == runtime.source.size() &&
          runtime.decoded == runtime.source,
          "game wrapper decompresses public LZO1X output");

    runtime.compressed[compressed_bytes - 1u] = 1u;
    Check(pal4::PackageCodec::Run(state, runtime) == 73 &&
          state.succeeded == 0u && runtime.reports == 1 &&
          !runtime.last_compressing && runtime.last_error != 0,
          "invalid LZO stream reaches game failure reporter");
    std::cout << "3 package MiniLZO boundary cases passed\n";
}
