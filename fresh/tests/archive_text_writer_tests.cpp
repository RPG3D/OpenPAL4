#include "pal4/archive_text_writer.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IArchiveStreamRuntime,
                      public pal4::IArchiveTextRuntime {
public:
    std::array<char, 5> text{'h', 'e', 'l', 'l', 'o'};
    char empty = '\0';
    std::uint32_t first_result = 0x101u;
    std::uint32_t second_result = 0x234u;
    std::uint32_t expected_length = 5u;
    bool expect_empty = false;
    std::vector<std::string> calls;

    std::uint32_t InvokeVirtual(std::uint32_t stream,
                                std::uint32_t slot,
                                std::uintptr_t buffer,
                                std::uint32_t bytes) override {
        Check(stream == 0x3000u && slot == 16u,
              "text uses write virtual slot");
        if (calls.empty()) {
            Check(bytes == 4u &&
                      *reinterpret_cast<const std::uint32_t*>(buffer) ==
                          expected_length,
                  "first write sends four-byte record length");
            calls.emplace_back("length");
            return first_result;
        }
        Check(bytes == expected_length &&
                  buffer == reinterpret_cast<std::uintptr_t>(
                      expect_empty ? static_cast<const void*>(&empty) :
                                     static_cast<const void*>(text.data())),
              "second write sends selected data pointer and byte count");
        calls.emplace_back("data");
        return second_result;
    }
    const void* DataAt(std::uint32_t address) override {
        Check(address == 0x4000u, "text data handle");
        calls.emplace_back("lookup");
        return text.data();
    }
    const void* EmptyData() override {
        calls.emplace_back("empty");
        return &empty;
    }
};

void Cases() {
    Runtime runtime;
    pal4::ArchiveStreamAdapter adapter(runtime);
    pal4::ArchiveStreamAdapterState stream;
    static_cast<void>(adapter.Construct(stream, 0x3000u, 0u));
    pal4::ArchiveTextWriter writer(adapter, runtime);
    pal4::ArchiveTextRecordState record{0xdeadbeefu, 0x4000u, 5u};

    runtime.first_result = 0x100u;
    Check(writer.Write(stream, record) == 0u &&
              runtime.calls == std::vector<std::string>{"length"},
          "zero low byte short-circuits data write");

    runtime.calls.clear();
    runtime.first_result = 0x101u;
    Check(writer.Write(stream, record) == 0x34u &&
              runtime.calls == std::vector<std::string>{
                  "length", "lookup", "data"} &&
              record.tag == 0xdeadbeefu,
          "nonempty record writes length then content and returns low byte");

    runtime.calls.clear();
    runtime.expect_empty = true;
    record.data = 0u;
    record.byte_count = 0u;
    runtime.expected_length = 0u;
    Check(writer.Write(stream, record) == 0x34u &&
              runtime.calls == std::vector<std::string>{
                  "length", "empty", "data"},
          "null data pointer uses static empty string even for zero bytes");
}

}  // namespace

int main() {
    try {
        Cases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
