#include "pal4/cegui_file_buffer_lifecycle.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::CeguiFileBufferRuntimeStub {
public:
    std::vector<std::string> calls;
    std::uint32_t pattern_file = 0u;
    std::uint32_t locale_calls = 0u;

    void Seed(std::uint32_t address, std::uint32_t value) {
        CeguiFileBufferRuntimeStub::WriteDword(address, value);
    }
    std::uint32_t Word(std::uint32_t address) {
        return CeguiFileBufferRuntimeStub::ReadDword(address);
    }
    std::uint8_t Byte(std::uint32_t address) {
        return CeguiFileBufferRuntimeStub::ReadByte(address);
    }
    std::uint32_t InitializeLocale() override {
        calls.emplace_back("locale_init");
        ++locale_calls;
        return 0x9000u;
    }
    std::uint32_t GlobalLocaleObject() override {
        return 0x9000u;
    }
    std::uint32_t GlobalLocaleValue() override {
        calls.emplace_back("fpos_global");
        return 0xabcd1234u;
    }
    void ConfigureFilePatterns(const std::uint32_t filebuf,
                               const std::uint32_t file) override {
        Check(filebuf == 0x4000u && Byte(filebuf + 72u) == 0u,
              "pattern setup sees constructed filebuf and clear flag");
        calls.emplace_back("pattern");
        pattern_file = file;
    }
};

void GetAndPutArea() {
    Runtime runtime;
    runtime.Seed(0x400cu, 0x8100u);
    runtime.Seed(0x401cu, 0x8104u);
    runtime.Seed(0x402cu, 0x8108u);
    runtime.Seed(0x4010u, 0x8200u);
    runtime.Seed(0x4020u, 0x8204u);
    runtime.Seed(0x4030u, 0x8208u);
    Check(pal4::CeguiFileBufferLifecycle::SetGetArea(
              0x4000u, 0x1000u, 0x1010u, 0x1030u, runtime)
              == 0x8108u && runtime.Word(0x8100u) == 0x1000u &&
              runtime.Word(0x8104u) == 0x1010u &&
              runtime.Word(0x8108u) == 0x20u,
          "setg writes through three pointer fields and returns count address");
    Check(pal4::CeguiFileBufferLifecycle::SetPutArea(
              0x4000u, 0x2000u, 0x2014u, runtime)
              == 0x8208u && runtime.Word(0x8200u) == 0x2000u &&
              runtime.Word(0x8204u) == 0x2000u &&
              runtime.Word(0x8208u) == 0x14u,
          "setp writes begin twice and computes unsigned byte count");
}

void ConstructWithAndWithoutFile() {
    Runtime runtime;
    runtime.Seed(0x9004u, 0u);
    Check(pal4::CeguiFileBufferLifecycle::ConstructFileBuffer(
              0x4000u, 0x7000u, runtime) == 0x4000u &&
              runtime.locale_calls == 3u && runtime.Word(0x9004u) == 2u &&
              runtime.Word(0x4000u) ==
                  pal4::CeguiFileBufferLifecycle::kFilebufVtable &&
              runtime.Word(0x4034u) == 0x9000u &&
              runtime.Word(0x404cu) == 0x9000u &&
              runtime.Word(0x400cu) == 0x4004u &&
              runtime.Word(0x4010u) == 0x4008u &&
              runtime.Word(0x401cu) == 0x4014u &&
              runtime.Word(0x4020u) == 0x4018u &&
              runtime.Word(0x402cu) == 0x4024u &&
              runtime.Word(0x4030u) == 0x4028u &&
              runtime.Word(0x4024u) == 0u &&
              runtime.Word(0x4028u) == 0u &&
              runtime.Word(0x4044u) == 0u &&
              runtime.Byte(0x4048u) == 0u &&
              runtime.Word(0x4050u) == 0x7000u &&
              runtime.Word(0x4040u) == 0xabcd1234u &&
              runtime.Word(0x403cu) == 0xabcd1234u &&
              runtime.Word(0x4038u) == 0u &&
              runtime.pattern_file == 0x7000u &&
              runtime.calls == std::vector<std::string>{
                  "locale_init", "locale_init", "locale_init", "pattern",
                  "fpos_global", "fpos_global"},
          "constructor sets pointers, three locale references, file and state");

    Runtime closed;
    closed.Seed(0x9004u, 0xffffffffu);
    Check(pal4::CeguiFileBufferLifecycle::ConstructFileBuffer(
              0x4000u, 0u, closed) == 0x4000u &&
              closed.Word(0x9004u) == 0xffffffffu &&
              closed.pattern_file == 0u &&
              closed.Word(0x4050u) == 0u &&
              closed.locale_calls == 3u,
          "null file skips pattern setup and sentinel locale count is stable");
}

void NullLocaleSlotAddress() {
    Runtime runtime;
    constexpr std::uint32_t object = 0xffffffb4u;
    runtime.Seed(0x9004u, 1u);
    Check(pal4::CeguiFileBufferLifecycle::ConstructFileBuffer(
              object, 0u, runtime) == object &&
              runtime.locale_calls == 2u &&
              runtime.Word(0x9004u) == 2u &&
              runtime.Word(0u) == 0x9000u,
          "wrapped null locale slot skips replacement initialization");
}

void StreambufAndPatternPointers() {
    Runtime runtime;
    Check(pal4::CeguiFileBufferLifecycle::InitializeStreambufPointers(
              0x4000u, runtime) == 0x4024u &&
              runtime.Word(0x400cu) == 0x4004u &&
              runtime.Word(0x4010u) == 0x4008u &&
              runtime.Word(0x401cu) == 0x4014u &&
              runtime.Word(0x4020u) == 0x4018u &&
              runtime.Word(0x402cu) == 0x4024u &&
              runtime.Word(0x4030u) == 0x4028u &&
              runtime.Word(0x4004u) == 0u &&
              runtime.Word(0x4008u) == 0u &&
              runtime.Word(0x4014u) == 0u &&
              runtime.Word(0x4018u) == 0u &&
              runtime.Word(0x4024u) == 0u &&
              runtime.Word(0x4028u) == 0u,
          "streambuf helper points six fields into object and clears targets");
    Check(pal4::CeguiFileBufferLifecycle::SetPatternPointers(
              0x4000u, 1u, 2u, 3u, 4u, 5u, 6u, runtime) == 3u &&
              runtime.Word(0x400cu) == 1u &&
              runtime.Word(0x4010u) == 4u &&
              runtime.Word(0x401cu) == 2u &&
              runtime.Word(0x4020u) == 5u &&
              runtime.Word(0x402cu) == 3u &&
              runtime.Word(0x4030u) == 6u,
          "pattern helper maps six arguments to get/put pointer fields");
}

}  // namespace

int main() {
    GetAndPutArea();
    ConstructWithAndWithoutFile();
    NullLocaleSlotAddress();
    StreambufAndPatternPointers();
    std::cout << "4 filebuf constructor test cases passed\n";
}
