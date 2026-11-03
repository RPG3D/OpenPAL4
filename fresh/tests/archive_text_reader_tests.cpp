#include "pal4/archive_text_reader.h"

#include <algorithm>
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
                      public pal4::IArchiveTextReadRuntime {
public:
    std::int32_t declared_length = 0;
    std::string payload;
    std::uint32_t first_result = 1u;
    std::uint32_t second_result = 1u;
    bool allocate_result = true;
    std::uint8_t header = 0u;
    std::string data;
    std::vector<std::string> events;
    std::uint32_t reads = 0u;

    std::uint32_t InvokeVirtual(std::uint32_t stream, std::uint32_t slot,
                                std::uintptr_t buffer,
                                std::uint32_t bytes) override {
        Check(stream == 0x3000u && slot == 12u, "read virtual slot");
        if (reads++ == 0u) {
            Check(bytes == 4u, "first read length");
            std::memcpy(reinterpret_cast<void*>(buffer), &declared_length, 4u);
            events.emplace_back("length");
            return first_result;
        }
        Check(bytes == static_cast<std::uint32_t>(declared_length),
              "second read byte count");
        Check(payload.size() == bytes, "payload has declared size");
        std::memcpy(reinterpret_cast<void*>(buffer), payload.data(), bytes);
        events.emplace_back("content");
        return second_result;
    }
    const char* EmptyData() override {
        events.emplace_back("empty");
        return "";
    }
    bool AllocateText(pal4::StringBufferState& text,
                      std::uint32_t length, std::uint8_t mode) override {
        Check(mode == 1u, "short path allocation mode");
        events.emplace_back("allocate");
        if (length == 0u) {
            text = {};
            return false;
        }
        if (allocate_result) {
            data.assign(length + 1u, '?');
            text.data = 0x1001u;
            text.capacity = length;
        }
        return allocate_result;
    }
    void SetErrorFlag(pal4::StringBufferState& text,
                      std::uint8_t flag) override {
        events.emplace_back(flag == 0u ? "clear" : "release");
        text = {};
    }
    void Reallocate(pal4::StringBufferState& text,
                    std::uint32_t length) override {
        events.emplace_back("reallocate");
        data.assign(length + 1u, '?');
        text.data = 0x1001u;
        text.capacity = length | 31u;
    }
    void SetTextSize(pal4::StringBufferState& text,
                     std::uint32_t length) override {
        events.emplace_back("size");
        text.length = length;
        data.at(length) = '\0';
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x1000u, "reference header address");
        events.emplace_back("header");
        return header;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        events.emplace_back("byte");
        if (address == 0x1000u) header = value;
        else {
            Check(address >= 0x1001u &&
                      address - 0x1001u < data.size(), "terminator address");
            data.at(address - 0x1001u) = static_cast<char>(value);
        }
    }
    void Copy(std::uint32_t destination, const char* source,
              std::uint32_t length) override {
        Check(destination == 0x1001u, "destination address");
        Check(data.size() > length, "destination capacity");
        events.emplace_back("copy");
        std::memcpy(data.data(), source, length);
    }
};

void Reset(Runtime& runtime, std::int32_t length, std::string payload) {
    runtime.declared_length = length;
    runtime.payload = std::move(payload);
    runtime.reads = 0u;
    runtime.events.clear();
    runtime.first_result = 1u;
    runtime.second_result = 1u;
    runtime.allocate_result = true;
}

void Cases() {
    Runtime runtime;
    pal4::ArchiveStreamAdapter adapter(runtime);
    pal4::ArchiveTextReader reader(adapter, runtime);
    pal4::ArchiveStreamAdapterState stream{0x3000u};
    pal4::StringBufferState text{0x1001u, 7u, 31u};

    Reset(runtime, 3, "abc");
    runtime.first_result = 0x100u;
    Check(reader.Read(stream, text) == 0u && runtime.reads == 1u &&
              text.length == 7u, "length read low byte failure");
    Reset(runtime, 3, "abc");
    runtime.second_result = 0x100u;
    Check(reader.Read(stream, text) == 0u && runtime.reads == 2u &&
              text.length == 7u, "content read low byte failure");
    Reset(runtime, -1, "");
    Check(reader.Read(stream, text) == 0u && runtime.reads == 1u,
          "malformed negative length rejected");

    Reset(runtime, 5, std::string("ab\0cd", 5));
    Check(reader.Read(stream, text) == 1u && runtime.reads == 2u &&
              text.length == 2u && runtime.data == std::string("ab\0", 3),
          "short text stops at embedded NUL");
    Reset(runtime, 0, "");
    Check(reader.Read(stream, text) == 1u && runtime.reads == 1u &&
              text.data == 0u &&
              runtime.events == std::vector<std::string>{"length", "empty", "allocate"},
          "zero length uses shared empty text");
    Reset(runtime, 2, "xy");
    text = {0x1001u, 7u, 31u};
    runtime.allocate_result = false;
    Check(reader.Read(stream, text) == 1u && text.length == 7u,
          "short allocation refusal still succeeds");

    Reset(runtime, 511, std::string(511, 's'));
    Check(reader.Read(stream, text) == 1u && text.length == 511u &&
              runtime.data.substr(0, 511) == runtime.payload,
          "511 bytes use short path");
    Reset(runtime, 512, std::string(512, 'l'));
    text = {};
    Check(reader.Read(stream, text) == 1u && text.length == 512u &&
              runtime.data.substr(0, 512) == runtime.payload &&
              runtime.events == std::vector<std::string>{
                  "length", "content", "release", "reallocate", "copy", "size"},
          "512 bytes use long path and resize");

    Reset(runtime, 512, std::string(512, 'x'));
    runtime.second_result = 0x100u;
    text = {0x1001u, 7u, 31u};
    Check(reader.Read(stream, text) == 0u && text.length == 7u &&
              runtime.events == std::vector<std::string>{"length", "content"},
          "failed long read leaves destination unchanged");
    Reset(runtime, 512, std::string(512, '\0'));
    runtime.header = 2u;
    Check(reader.Read(stream, text) == 1u && text.data == 0u &&
              runtime.header == 1u &&
              runtime.events == std::vector<std::string>{
                  "length", "content", "header", "byte", "clear"},
          "empty long text decrements shared reference");
    Reset(runtime, 512, std::string(512, '\0'));
    text = {0x1001u, 7u, 31u};
    runtime.header = 0xffu;
    Check(reader.Read(stream, text) == 1u && text.data == 0u &&
              runtime.events.back() == "release",
          "empty long text releases unshared buffer");
    Reset(runtime, 512, std::string("hello\0", 6) + std::string(506, 'z'));
    text = {0x1001u, 7u, 31u};
    runtime.header = 2u;
    Check(reader.Read(stream, text) == 1u && text.length == 5u &&
              runtime.events == std::vector<std::string>{
                  "length", "content", "header", "reallocate", "copy", "size"},
          "shared long text detaches before copy");
    Reset(runtime, 512, std::string("hello\0", 6) + std::string(506, 'z'));
    text = {0x1001u, 7u, 31u};
    runtime.header = 0u;
    Check(reader.Read(stream, text) == 1u && text.length == 5u &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "reallocate") == runtime.events.end(),
          "unshared small capacity is reused");

    pal4::ArchiveTextReadRuntimeStub stub;
    pal4::StringBufferState empty;
    Check(std::strcmp(stub.EmptyData(), "") == 0 &&
              !stub.AllocateText(empty, 0u, 1u), "native stub links");
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
