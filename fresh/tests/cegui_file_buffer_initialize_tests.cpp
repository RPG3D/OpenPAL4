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
    std::uint32_t GlobalLocaleObject() override { return 0x9000u; }
    std::uint32_t GlobalLocaleValue() override { return 0x12345678u; }
    std::uint32_t DestroyReference(std::uint32_t object,
                                   std::uint8_t flags) override {
        Check(object == 0x9000u && flags == 1u,
              "last locale reference uses deleting destructor");
        calls.emplace_back("destroy_ref");
        return object;
    }
};

void DefaultLocaleHelper() {
    Runtime runtime;
    runtime.Seed(0x9004u, 4u);
    Check(pal4::CeguiFileBufferLifecycle::InitializeDefaultLocale(
              0x8000u, runtime) == 0x8000u &&
              runtime.Word(0x8000u) == 0x9000u &&
              runtime.Word(0x9004u) == 5u &&
              runtime.locale_calls == 1u,
          "locale helper stores global object and increments its reference");
    runtime.Seed(0x9004u, 0xffffffffu);
    Check(pal4::CeguiFileBufferLifecycle::InitializeDefaultLocale(
              0x8004u, runtime) == 0x8004u &&
              runtime.Word(0x9004u) == 0xffffffffu,
          "sentinel locale count remains unchanged");
}

void ModeZeroReplacesLocale() {
    Runtime runtime;
    runtime.Seed(0x9004u, 2u);
    runtime.Seed(0x404cu, 0x9000u);
    runtime.Seed(0x4044u, 0xaaaau);
    Check(pal4::CeguiFileBufferLifecycle::Initialize(
              0x4000u, 0x7000u, 0u, runtime) == 0x12345678u &&
              runtime.Byte(0x4048u) == 0u &&
              runtime.Word(0x404cu) == 0x9000u &&
              runtime.Word(0x9004u) == 2u &&
              runtime.locale_calls == 1u &&
              runtime.Word(0x4044u) == 0u &&
              runtime.Word(0x401cu) == 0x7000u &&
              runtime.Word(0x400cu) == 0x7008u &&
              runtime.Word(0x4010u) == 0x7008u &&
              runtime.Word(0x4020u) == 0x7000u &&
              runtime.Word(0x402cu) == 0x7004u &&
              runtime.Word(0x4030u) == 0x7004u &&
              runtime.Word(0x4050u) == 0x7000u &&
              runtime.Word(0x4040u) == 0x12345678u &&
              runtime.Word(0x403cu) == 0x12345678u &&
              runtime.Word(0x4038u) == 0u,
          "mode zero releases and reacquires locale, then maps CRT buffer");
}

void OtherModesAndNullFile() {
    Runtime runtime;
    runtime.Seed(0x9004u, 3u);
    runtime.Seed(0x404cu, 0x9000u);
    runtime.Seed(0x4044u, 0xaaaau);
    Check(pal4::CeguiFileBufferLifecycle::Initialize(
              0x4000u, 0x7000u, 1u, runtime) == 0x12345678u &&
              runtime.Byte(0x4048u) == 1u &&
              runtime.Word(0x401cu) == 0x4014u &&
              runtime.Word(0x4050u) == 0x7000u &&
              runtime.Word(0x4044u) == 0xaaaau &&
              runtime.Word(0x9004u) == 3u &&
              runtime.locale_calls == 0u,
          "mode one retains locale and internal buffer pointers");
    Check(pal4::CeguiFileBufferLifecycle::Initialize(
              0x4000u, 0u, 2u, runtime) == 0x12345678u &&
              runtime.Byte(0x4048u) == 0u &&
              runtime.Word(0x401cu) == 0x4014u &&
              runtime.Word(0x4050u) == 0u &&
              runtime.Word(0x4044u) == 0xaaaau &&
              runtime.locale_calls == 0u,
          "other mode with null FILE does not touch locale or redirect pointers");
}

}  // namespace

int main() {
    DefaultLocaleHelper();
    ModeZeroReplacesLocale();
    OtherModesAndNullFile();
    std::cout << "3 filebuf initialize test cases passed\n";
}
