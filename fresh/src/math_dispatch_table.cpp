#include "pal4/math_dispatch_table.h"

#include <array>

namespace pal4 {

void MathDispatchInitializer::Initialize(MathDispatchTable& table) {
    table.entries[8u] = 0x601c30u;
    table.entries[9u] = 0x601c60u;
    table.entries[0u] = 0x601a00u;
    table.entries[1u] = 0x601a50u;
    table.entries[2u] = 0x601aa0u;
    table.entries[3u] = 0x601b00u;
    table.entries[4u] = 0x601b70u;
    table.entries[5u] = 0x601b80u;
    table.entries[6u] = 0x601b90u;
    table.entries[7u] = 0x601be0u;
}

void MathDispatchInitializer::InstallSse(MathDispatchTable& table) {
    table.entries[8u] = 0x601c50u;
    table.entries[9u] = 0x601c80u;
    table.entries[0u] = 0x601a10u;
    table.entries[1u] = 0x601a60u;
    table.entries[2u] = 0x601ab0u;
    table.entries[3u] = 0x601b60u;
    table.entries[6u] = 0x601ba0u;
    table.entries[7u] = 0x601bf0u;
}

std::uint8_t MathRuntimeInitializer::Initialize(
    MathCpuFeatures& features, MathDispatchTable& table,
    IMathRuntimeInitialization& runtime) {
    std::array<char, 256u> brand{};
    std::array<char, 256u> serial{};
    runtime.CpuBrand(brand.data(), static_cast<std::uint32_t>(brand.size()));
    runtime.CpuSerial(1u, serial.data(),
                      static_cast<std::uint32_t>(serial.size()));
    features.mmx = runtime.HasMmx();
    features.sse = runtime.HasSse();
    features.sse2 = runtime.HasSse2();
    features.three_d_now = runtime.Has3DNow();

    constexpr auto report =
        "\nCPU-DETECTION:\n\tProcessor:%s\n\tSerialNumber:\t\t%s\n"
        "\tMMX:  %d, SSE:  %d, SSE2:  %d, 3DNow:  %d\n\n\n";
    const auto logger = runtime.InitializeLogger();
    runtime.AppendCpuReport(logger, report, brand.data(), serial.data(),
                            features);
    runtime.InitializeSquareRootTables();
    MathDispatchInitializer::Initialize(table);
    if (features.sse != 0u) MathDispatchInitializer::InstallSse(table);
    return features.sse;
}

}  // namespace pal4
