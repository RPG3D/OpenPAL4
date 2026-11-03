#include "pal4/cegui_stream_buffer_operations.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::CeguiStreamBufferRuntimeStub {
public:
    std::vector<std::string> calls;
    std::vector<std::uint32_t> copies;
    std::vector<std::uint8_t> overflow_values;
    std::vector<std::int32_t> uflow_values;
    std::int32_t underflow_result = -1;
    std::int32_t overflow_result = -1;
    std::int32_t applied_result = 0;

    std::int32_t Underflow(std::uint32_t object) override {
        Check(object == 0x4000u, "underflow receives streambuf object");
        calls.emplace_back("underflow");
        return underflow_result;
    }
    std::int32_t ApplyUnderflowResult(std::uint32_t object,
                                      std::int32_t value) override {
        Check(object == 0x4000u && value == underflow_result,
              "second virtual slot receives underflow result");
        calls.emplace_back("apply");
        return applied_result;
    }
    std::int32_t Uflow(std::uint32_t object) override {
        Check(object == 0x4000u, "uflow receives streambuf object");
        calls.emplace_back("uflow");
        if (uflow_values.empty()) {
            return -1;
        }
        const auto value = uflow_values.front();
        uflow_values.erase(uflow_values.begin());
        return value;
    }
    std::int32_t Overflow(std::uint32_t object,
                          std::uint8_t value) override {
        Check(object == 0x4000u, "overflow receives streambuf object");
        calls.emplace_back("overflow");
        overflow_values.push_back(value);
        return overflow_result;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t bytes) override {
        calls.emplace_back("copy");
        copies.push_back(bytes);
        pal4::CeguiStreamBufferRuntimeStub::CopyBytes(
            destination, source, bytes);
    }
    std::uint32_t FposLocale() override {
        calls.emplace_back("locale");
        return 0xfaceu;
    }
    void SeedWord(std::uint32_t address, std::uint32_t value) {
        pal4::CeguiStreamBufferRuntimeStub::WriteDword(address, value);
    }
    void SeedByte(std::uint32_t address, std::uint8_t value) {
        pal4::CeguiStreamBufferRuntimeStub::WriteByte(address, value);
    }
    std::uint32_t Word(std::uint32_t address) {
        return pal4::CeguiStreamBufferRuntimeStub::ReadDword(address);
    }
    std::uint8_t Byte(std::uint32_t address) {
        return pal4::CeguiStreamBufferRuntimeStub::ReadByte(address);
    }
};

void SeedReadCursor(Runtime& runtime, std::uint32_t cursor,
                    std::uint32_t count) {
    runtime.SeedWord(0x401cu, 0x7200u);
    runtime.SeedWord(0x7200u, cursor);
    runtime.SeedWord(0x402cu, 0x7100u);
    runtime.SeedWord(0x7100u, count);
}

void SeedWriteCursor(Runtime& runtime, std::uint32_t cursor,
                     std::uint32_t count) {
    runtime.SeedWord(0x4020u, 0x7300u);
    runtime.SeedWord(0x7300u, cursor);
    runtime.SeedWord(0x4030u, 0x7400u);
    runtime.SeedWord(0x7400u, count);
}

void ReadCharUsesUnderflowAndAdvancesCursor() {
    Runtime runtime;
    Check(pal4::CeguiStreamBufferOperations::ReadChar(
              0x4000u, runtime) == -1 &&
              runtime.calls == std::vector<std::string>{"underflow"},
          "EOF underflow skips cursor access");
    runtime.calls.clear();
    runtime.underflow_result = 65;
    SeedReadCursor(runtime, 0x8000u, 2u);
    runtime.SeedByte(0x8000u, 0xe5u);
    Check(pal4::CeguiStreamBufferOperations::ReadChar(
              0x4000u, runtime) == 0xe5 &&
              runtime.Word(0x7100u) == 1u &&
              runtime.Word(0x7200u) == 0x8001u &&
              runtime.calls == std::vector<std::string>{"underflow"},
          "successful underflow decrements count and reads unsigned byte");
}

void ReadBytesCopiesThenFallsBackToUflow() {
    Runtime runtime;
    SeedReadCursor(runtime, 0x8000u, 3u);
    runtime.SeedByte(0x8000u, 'A');
    runtime.SeedByte(0x8001u, 'B');
    runtime.SeedByte(0x8002u, 'C');
    runtime.uflow_values = {'D', -1};
    Check(pal4::CeguiStreamBufferOperations::ReadBytes(
              0x4000u, 0x9000u, 5, runtime) == 4u &&
              runtime.Byte(0x9000u) == 'A' &&
              runtime.Byte(0x9001u) == 'B' &&
              runtime.Byte(0x9002u) == 'C' &&
              runtime.Byte(0x9003u) == 'D' &&
              runtime.Word(0x7100u) == 0u &&
              runtime.Word(0x7200u) == 0x8003u &&
              runtime.copies == std::vector<std::uint32_t>{3u} &&
              runtime.calls == std::vector<std::string>{
                  "copy", "uflow", "uflow"},
          "read copies get area then returns partial count at EOF");
}

void ReadBytesWithNullCursorUsesUflow() {
    Runtime runtime;
    SeedReadCursor(runtime, 0u, 99u);
    Check(pal4::CeguiStreamBufferOperations::ReadBytes(
              0x4000u, 0x9000u, 3, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"uflow"},
          "null get cursor bypasses count and returns zero at EOF");
    runtime.calls.clear();
    Check(pal4::CeguiStreamBufferOperations::ReadBytes(
              0x4000u, 0x9000u, -1, runtime) == 0u &&
              runtime.calls.empty(),
          "nonpositive read request has no side effects");
}

void WriteBytesCopiesThenFallsBackToOverflow() {
    Runtime runtime;
    SeedWriteCursor(runtime, 0xa000u, 2u);
    runtime.SeedByte(0xb000u, 'X');
    runtime.SeedByte(0xb001u, 'Y');
    runtime.SeedByte(0xb002u, 'Z');
    runtime.overflow_result = 'Z';
    Check(pal4::CeguiStreamBufferOperations::WriteBytes(
              0x4000u, 0xb000u, 3, runtime) == 3u &&
              runtime.Byte(0xa000u) == 'X' &&
              runtime.Byte(0xa001u) == 'Y' &&
              runtime.Word(0x7400u) == 0u &&
              runtime.Word(0x7300u) == 0xa002u &&
              runtime.overflow_values ==
                  std::vector<std::uint8_t>{'Z'} &&
              runtime.calls == std::vector<std::string>{
                  "copy", "overflow"},
          "write copies put area then overflows the final byte");
    runtime.calls.clear();
    runtime.overflow_result = -1;
    Check(pal4::CeguiStreamBufferOperations::WriteBytes(
              0x4000u, 0xb002u, 1, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"overflow"},
          "failed overflow returns the completed byte count");
}

void FailurePositionsAndSetBufferPreserveUnwrittenData() {
    Runtime runtime;
    runtime.SeedWord(0xc004u, 0xdeadbeefu);
    Check(pal4::CeguiStreamBufferOperations::SeekOffFailure(
              0xc000u, runtime) == 0xc000u &&
              runtime.Word(0xc000u) == 0xffffffffu &&
              runtime.Word(0xc004u) == 0xdeadbeefu &&
              runtime.Word(0xc008u) == 0u &&
              runtime.Word(0xc00cu) == 0u &&
              runtime.Word(0xc010u) == 0xfaceu &&
              runtime.calls == std::vector<std::string>{"locale"},
          "seekoff failure writes only four observed DWORDs");
    runtime.calls.clear();
    Check(pal4::CeguiStreamBufferOperations::InitializeFailurePosition(
              0xc000u, runtime) == 0xc000u &&
              runtime.Word(0xc004u) == 0xdeadbeefu &&
              runtime.calls == std::vector<std::string>{"locale"},
          "second failure-position initializer shares field behavior");
    Check(pal4::CeguiStreamBufferOperations::SetBuffer(
              0x4000u, 0x1234u, 99u) == 0x4000u,
          "set-buffer placeholder returns the streambuf unchanged");
}

void PeekUsesGetAreaOrBothVirtualSlots() {
    Runtime runtime;
    SeedReadCursor(runtime, 0x8000u, 1u);
    runtime.SeedByte(0x8000u, 0xe5u);
    Check(pal4::CeguiStreamBufferOperations::PeekChar(
              0x4000u, runtime) == 0xe5 && runtime.calls.empty() &&
              runtime.Word(0x7200u) == 0x8000u &&
              runtime.Word(0x7100u) == 1u,
          "nonempty get area returns unsigned byte without advancing");
    runtime.SeedWord(0x7100u, 0u);
    runtime.underflow_result = 32;
    runtime.applied_result = 41;
    Check(pal4::CeguiStreamBufferOperations::PeekChar(
              0x4000u, runtime) == 41 &&
              runtime.calls == std::vector<std::string>{
                  "underflow", "apply"},
          "empty get area invokes virtual slots in order");
    runtime.calls.clear();
    SeedReadCursor(runtime, 0xfffffffEu, 3u);
    Check(pal4::CeguiStreamBufferOperations::PeekChar(
              0x4000u, runtime) == 41 &&
              runtime.calls == std::vector<std::string>{
                  "underflow", "apply"},
          "wrapped cursor plus count falls back to virtual slots");
}

}  // namespace

int main() {
    ReadCharUsesUnderflowAndAdvancesCursor();
    ReadBytesCopiesThenFallsBackToUflow();
    ReadBytesWithNullCursorUsesUflow();
    WriteBytesCopiesThenFallsBackToOverflow();
    FailurePositionsAndSetBufferPreserveUnwrittenData();
    PeekUsesGetAreaOrBothVirtualSlots();
    std::cout << "6 CEGUI stream-buffer operation test cases passed\n";
}
