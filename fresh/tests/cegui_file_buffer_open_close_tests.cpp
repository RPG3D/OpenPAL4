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
    std::uint32_t opened_file = 0x7000u;
    std::int32_t close_result = 0;
    bool force_external_buffer = false;

    void Seed(std::uint32_t address, std::uint32_t value) {
        CeguiFileBufferRuntimeStub::WriteDword(address, value);
    }
    std::uint32_t Word(std::uint32_t address) {
        return CeguiFileBufferRuntimeStub::ReadDword(address);
    }
    std::uint8_t Byte(std::uint32_t address) {
        return CeguiFileBufferRuntimeStub::ReadByte(address);
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        if (force_external_buffer && address == 0x4048u) {
            return 0u;
        }
        return CeguiFileBufferRuntimeStub::ReadByte(address);
    }
    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        if (address == 0x400cu) {
            calls.emplace_back("first_pointer");
        }
        CeguiFileBufferRuntimeStub::WriteDword(address, value);
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        if (address == 0x4048u) {
            calls.emplace_back(value == 1u ? "mark_open" : "mark_closed");
        }
        CeguiFileBufferRuntimeStub::WriteByte(address, value);
    }
    std::uint32_t OpenCrtFile(const std::uint32_t name,
                              const std::uint32_t mode) override {
        Check(name == 0x8100u && mode == 0x12u,
              "CRT open receives name and mode");
        calls.emplace_back("fopen");
        return opened_file;
    }
    std::int32_t CloseCrtFile(const std::uint32_t file) override {
        Check(file == 0x7000u, "CRT close receives current file");
        calls.emplace_back("fclose");
        return close_result;
    }
    std::uint32_t GlobalLocaleValue() override {
        return 0xdeadbeefu;
    }
    void AcquireCodecvtFacet(const std::uint32_t object) override {
        Check(object == 0x4000u && Word(object + 80u) == 0x7000u &&
                  Word(object + 56u) == 0u,
              "codecvt lookup occurs after file and state writes");
        calls.emplace_back("codecvt");
    }
};

void FailedOpenLeavesStateUntouched() {
    Runtime runtime;
    runtime.Seed(0x4050u, 0x7000u);
    Check(pal4::CeguiFileBufferLifecycle::Open(
              0x4000u, 0x8100u, 0x12u, runtime) == 0u &&
              runtime.calls.empty() && runtime.Word(0x4050u) == 0x7000u,
          "already-open filebuf does not call CRT open");
    runtime.Seed(0x4050u, 0u);
    runtime.opened_file = 0u;
    Check(pal4::CeguiFileBufferLifecycle::Open(
              0x4000u, 0x8100u, 0x12u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"fopen"} &&
              runtime.Word(0x4050u) == 0u,
          "failed CRT open leaves state untouched");
}

void OpenAndClose() {
    Runtime runtime;
    runtime.Seed(0x4050u, 0u);
    Check(pal4::CeguiFileBufferLifecycle::Open(
              0x4000u, 0x8100u, 0x12u, runtime) == 0x4000u &&
              runtime.calls.front() == "fopen" &&
              runtime.calls[1] == "first_pointer" &&
              runtime.calls[2] == "mark_open" &&
              runtime.calls.back() == "codecvt" &&
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
              runtime.Word(0x4028u) == 0u &&
              runtime.Byte(0x4048u) == 1u &&
              runtime.Word(0x4050u) == 0x7000u &&
              runtime.Word(0x4040u) == 0xdeadbeefu &&
              runtime.Word(0x403cu) == 0xdeadbeefu &&
              runtime.Word(0x4038u) == 0u,
          "successful open restores internal areas and acquires codecvt last");

    runtime.calls.clear();
    runtime.close_result = -1;
    Check(pal4::CeguiFileBufferLifecycle::Close(
              0x4000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"fclose"} &&
              runtime.Word(0x4050u) == 0x7000u &&
              runtime.Byte(0x4048u) == 1u,
          "failed close preserves file and buffer state");
    runtime.calls.clear();
    runtime.close_result = 0;
    Check(pal4::CeguiFileBufferLifecycle::Close(
              0x4000u, runtime) == 0x4000u &&
              runtime.calls.front() == "fclose" &&
              runtime.calls[1] == "mark_closed" &&
              runtime.calls[2] == "first_pointer" &&
              runtime.Word(0x4050u) == 0u &&
              runtime.Byte(0x4048u) == 0u &&
              runtime.Word(0x4040u) == 0xdeadbeefu &&
              runtime.Word(0x4038u) == 0u,
          "successful close resets file and pointers after CRT call");
    runtime.calls.clear();
    Check(pal4::CeguiFileBufferLifecycle::Close(
              0x4000u, runtime) == 0u && runtime.calls.empty(),
          "closed filebuf skips CRT close");
}

void ExternalBufferBranch() {
    Runtime runtime;
    runtime.Seed(0x4050u, 0u);
    runtime.force_external_buffer = true;
    Check(pal4::CeguiFileBufferLifecycle::Open(
              0x4000u, 0x8100u, 0x12u, runtime) == 0x4000u &&
              runtime.Word(0x400cu) == 0x7008u &&
              runtime.Word(0x4010u) == 0x7008u &&
              runtime.Word(0x401cu) == 0x7000u &&
              runtime.Word(0x4020u) == 0x7000u &&
              runtime.Word(0x402cu) == 0x7004u &&
              runtime.Word(0x4030u) == 0x7004u,
          "external CRT buffer branch redirects six pointers");
}

}  // namespace

int main() {
    FailedOpenLeavesStateUntouched();
    OpenAndClose();
    ExternalBufferBranch();
    std::cout << "3 filebuf open/close test cases passed\n";
}
