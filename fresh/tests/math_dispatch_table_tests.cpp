#include "pal4/math_dispatch_table.h"

#include <array>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMathRuntimeInitialization {
    std::vector<std::string> events;
    std::uint8_t sse = 0u;

    void CpuBrand(char* output, const std::uint32_t capacity) override {
        Check(capacity == 256u && output[0u] == '\0' &&
                  output[255u] == '\0',
              "brand output starts as a zeroed 256-byte buffer");
        events.emplace_back("brand");
        std::copy_n("Test CPU", 9u, output);
    }
    void CpuSerial(const std::uint32_t selector, char* output,
                   const std::uint32_t capacity) override {
        Check(selector == 1u && capacity == 256u &&
                  output[0u] == '\0' && output[255u] == '\0',
              "serial query uses selector one and zeroed buffer");
        events.emplace_back("serial");
        std::copy_n("ABC", 4u, output);
    }
    std::uint8_t HasMmx() override {
        events.emplace_back("mmx");
        return 1u;
    }
    std::uint8_t HasSse() override {
        events.emplace_back("sse");
        return sse;
    }
    std::uint8_t HasSse2() override {
        events.emplace_back("sse2");
        return 2u;
    }
    std::uint8_t Has3DNow() override {
        events.emplace_back("3dnow");
        return 3u;
    }
    std::uint32_t InitializeLogger() override {
        events.emplace_back("logger");
        return 0x6000u;
    }
    void AppendCpuReport(const std::uint32_t logger,
                         const char* format, const char* brand,
                         const char* serial,
                         const pal4::MathCpuFeatures& features) override {
        Check(logger == 0x6000u &&
                  std::string(brand) == "Test CPU" &&
                  std::string(serial) == "ABC" &&
                  std::string(format) ==
                      "\nCPU-DETECTION:\n\tProcessor:%s\n"
                      "\tSerialNumber:\t\t%s\n\tMMX:  %d, SSE:  %d, "
                      "SSE2:  %d, 3DNow:  %d\n\n\n" &&
                  features.mmx == 1u && features.sse == sse &&
                  features.sse2 == 2u && features.three_d_now == 3u,
              "CPU report receives native format, text and feature bytes");
        events.emplace_back("append");
    }
    void InitializeSquareRootTables() override {
        events.emplace_back("sqrt-tables");
    }
};

}  // namespace

int main() {
    pal4::MathDispatchTable table{};
    table.entries.fill(0xffffffffu);
    pal4::MathDispatchInitializer::Initialize(table);
    constexpr std::array<std::uint32_t, 10u> expected{
        0x601a00u, 0x601a50u, 0x601aa0u, 0x601b00u,
        0x601b70u, 0x601b80u, 0x601b90u, 0x601be0u,
        0x601c30u, 0x601c60u};
    Check(table.entries == expected &&
              pal4::MathDispatchTable::kNativeAddress == 0x93cd74u &&
              table.entries[pal4::MathDispatchTable::kTangentSlot] == 0x601b90u &&
              table.entries[pal4::MathDispatchTable::kArctangentSlot] == 0x601be0u,
          "initialization sets all native scalar-math dispatch slots");
    pal4::MathDispatchInitializer::Initialize(table);
    Check(table.entries == expected,
          "repeated initialization restores the same complete table");

    Runtime runtime;
    pal4::MathCpuFeatures features{};
    Check(pal4::MathRuntimeInitializer::Initialize(
              features, table, runtime) == 0u &&
              table.entries == expected &&
              runtime.events == std::vector<std::string>{
                  "brand", "serial", "mmx", "sse", "sse2", "3dnow",
                  "logger", "append", "sqrt-tables"},
          "without SSE initialization retains scalar dispatch order");

    runtime.events.clear();
    runtime.sse = 1u;
    constexpr std::array<std::uint32_t, 10u> expected_sse{
        0x601a10u, 0x601a60u, 0x601ab0u, 0x601b60u,
        0x601b70u, 0x601b80u, 0x601ba0u, 0x601bf0u,
        0x601c50u, 0x601c80u};
    Check(pal4::MathRuntimeInitializer::Initialize(
              features, table, runtime) == 1u &&
              table.entries == expected_sse &&
              table.entries[pal4::MathDispatchTable::kTangentSlot] == 0x601ba0u &&
              table.entries[pal4::MathDispatchTable::kArctangentSlot] == 0x601bf0u &&
              features.mmx == 1u && features.sse == 1u &&
              features.sse2 == 2u && features.three_d_now == 3u &&
              runtime.events == std::vector<std::string>{
                  "brand", "serial", "mmx", "sse", "sse2", "3dnow",
                  "logger", "append", "sqrt-tables"},
          "SSE initialization replaces eight slots and preserves two inverse functions");
}
