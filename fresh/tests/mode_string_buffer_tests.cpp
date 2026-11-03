#include "pal4/mode_string_buffer.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IModeStringBufferRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint8_t> bytes;
    bool force_length{};
    std::uint32_t forced_length{};
    std::uint8_t flag{};

    std::uint32_t CStringLength(const char* source) override {
        calls.emplace_back("length");
        return force_length ? forced_length :
            static_cast<std::uint32_t>(std::strlen(source));
    }
    [[noreturn]] void ThrowLength() override {
        calls.emplace_back("throw");
        throw std::length_error("too long");
    }
    void SetErrorFlag(pal4::ModeStringBufferRecord& record,
                      std::uint8_t value) override {
        Check(record.buffer.data == 0u && record.buffer.length == 0u &&
                  record.buffer.capacity == 0u,
              "initial record fields cleared before flag call");
        flag = value;
        calls.emplace_back("flag");
    }
    void Reallocate(pal4::ModeStringBufferRecord& record,
                    std::uint32_t length) override {
        Check(length == 3u && record.buffer.data == 0u,
              "new record always reallocates nonempty text");
        record.buffer.data = 0x2000u;
        record.buffer.capacity = length;
        calls.emplace_back("allocate");
    }
    void Copy(std::uint32_t destination, const char* source,
              std::uint32_t length) override {
        Check(destination == 0x2000u && length == 3u,
              "copy address and length");
        for (std::uint32_t i = 0u; i < length; ++i) {
            bytes[destination + i] = static_cast<std::uint8_t>(source[i]);
        }
        calls.emplace_back("copy");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("terminator");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::ModeStringBufferRecord record{};
    record.mode_word = 0xaabbccddu;
    record.buffer = {0x1234u, 7u, 8u};
    Check(&pal4::ModeStringBuffer::InitializeCString(
              record, "", 0x34u, runtime) == &record &&
              record.mode_word == 0xaabbcc34u &&
              record.buffer.data == 0u && record.buffer.length == 0u &&
              record.buffer.capacity == 0u && runtime.flag == 1u &&
              runtime.calls == std::vector<std::string>{"length", "flag"},
          "empty string initializes mode byte and zero record");

    runtime.calls.clear();
    Check(&pal4::ModeStringBuffer::InitializeCString(
              record, "abc", 0x56u, runtime) == &record &&
              record.mode_word == 0xaabbcc56u &&
              record.buffer.data == 0x2000u &&
              record.buffer.length == 3u &&
              record.buffer.capacity == 3u &&
              runtime.ReadByte(0x2000u) == 'a' &&
              runtime.ReadByte(0x2001u) == 'b' &&
              runtime.ReadByte(0x2002u) == 'c' &&
              runtime.ReadByte(0x2003u) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "length", "flag", "allocate", "copy", "terminator"},
          "nonempty string allocates copies and terminates");

    runtime.calls.clear();
    runtime.force_length = true;
    runtime.forced_length = 0xfffffffeu;
    try {
        (void)pal4::ModeStringBuffer::InitializeCString(
            record, "x", 0x78u, runtime);
        Check(false, "oversized string throws");
    } catch (const std::length_error&) {
        Check(record.mode_word == 0xaabbcc78u &&
                  record.buffer.data == 0u &&
                  runtime.calls == std::vector<std::string>{
                      "length", "throw"},
              "length gate runs before flag and allocation");
    }

    pal4::ModeStringBufferRuntimeStub stub;
    pal4::ModeStringBufferRecord linked{};
    (void)pal4::ModeStringBuffer::InitializeCString(
        linked, "linked", 0xabu, stub);
    Check(linked.buffer.length == 6u &&
              stub.ReadByte(linked.buffer.data + 6u) == 0u,
          "default runtime allocates a readable zero-terminated buffer");
}
