#include "pal4/package_codec.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IPackageCodecRuntime {
public:
    std::int32_t Compress(std::uint32_t source, std::uint32_t source_bytes,
                          std::uint32_t destination,
                          std::uint32_t& result_bytes) override {
        ++compress_calls;
        Capture(source, source_bytes, destination);
        result_bytes = output_bytes;
        return lzo_result;
    }
    std::int32_t Decompress(std::uint32_t source, std::uint32_t source_bytes,
                            std::uint32_t destination,
                            std::uint32_t& result_bytes) override {
        ++decompress_calls;
        Capture(source, source_bytes, destination);
        result_bytes = output_bytes;
        return lzo_result;
    }
    std::int32_t ReportFailure(bool compress, std::int32_t result) override {
        ++reports;
        last_compress = compress;
        last_error = result;
        return report_result;
    }
    void Capture(std::uint32_t source, std::uint32_t bytes,
                 std::uint32_t destination) {
        last_source = source;
        last_bytes = bytes;
        last_destination = destination;
    }

    int compress_calls = 0;
    int decompress_calls = 0;
    int reports = 0;
    bool last_compress = false;
    std::int32_t lzo_result = 0;
    std::int32_t last_error = 0;
    std::int32_t report_result = 37;
    std::uint32_t output_bytes = 48u;
    std::uint32_t last_source = 0;
    std::uint32_t last_bytes = 0;
    std::uint32_t last_destination = 0;
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::PackageCodecState state;
    state.mode = 1u;
    state.source_bytes = 123u;
    state.result_bytes = 9u;
    Check(pal4::PackageCodec::Run(state, runtime) == 0 &&
          state.result_bytes == 123u && state.succeeded == 1u &&
          runtime.compress_calls == 0 && runtime.decompress_calls == 0,
          "uncompressed mode copies the input length without library call");

    state.mode = 2u;
    state.compress = 1u;
    state.source = 0x123400u;
    state.destination = 0x567800u;
    Check(pal4::PackageCodec::Run(state, runtime) == 0 &&
          state.result_bytes == 48u && state.succeeded == 1u &&
          runtime.compress_calls == 1 && runtime.last_source == state.source &&
          runtime.last_bytes == 123u &&
          runtime.last_destination == state.destination,
          "compression forwards the five-argument library contract");
    runtime.lzo_result = -4;
    runtime.report_result = 91;
    Check(pal4::PackageCodec::Run(state, runtime) == 91 &&
          state.succeeded == 0u && runtime.reports == 1 &&
          runtime.last_compress && runtime.last_error == -4,
          "compression failure returns message-box result");

    state.compress = 0u;
    runtime.lzo_result = 0;
    runtime.output_bytes = 321u;
    Check(pal4::PackageCodec::Run(state, runtime) == 0 &&
          state.result_bytes == 321u && state.succeeded == 1u &&
          runtime.decompress_calls == 1,
          "decompression forwards output length and marks success");
    runtime.lzo_result = -8;
    Check(pal4::PackageCodec::Run(state, runtime) == 91 &&
          state.succeeded == 0u && runtime.reports == 2 &&
          !runtime.last_compress && runtime.last_error == -8,
          "decompression failure returns message-box result");

    state.mode = 4u;
    state.succeeded = 7u;
    Check(pal4::PackageCodec::Run(state, runtime) == 2 &&
          state.succeeded == 7u && runtime.reports == 2,
          "unsupported mode returns mode minus two without state change");
    state.mode = 0u;
    Check(pal4::PackageCodec::Run(state, runtime) == -2 &&
          state.succeeded == 7u,
          "unsupported mode uses 32-bit wraparound subtraction");
    std::cout << "7 package codec cases passed\n";
}
