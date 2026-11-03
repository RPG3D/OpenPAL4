#include "pal4/trade_window_string_formatter.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ITradeWindowStringFormatRuntime {
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::string result = "value=42";
    bool allocate = true;
    bool fail_assign = false;
    std::uint8_t initial_reference = 0u;
    std::uint32_t formatted_with = 0u;
    std::uint32_t arguments_at = 0u;

    std::string Vsprintf(std::uint32_t format, std::uint32_t arguments,
                        std::uint32_t buffer_bytes) override {
        formatted_with = format;
        arguments_at = arguments;
        Check(buffer_bytes == 1024u, "native stack buffer capacity");
        calls.emplace_back("format");
        return result;
    }
    bool AllocateTemporary(pal4::StringBufferState& state,
                           std::uint32_t length,
                           std::uint8_t mode) override {
        Check(length == result.size() && mode == 1u,
              "temporary allocation size and mode");
        calls.emplace_back("allocate");
        if (!allocate) return false;
        state = {0x5001u, 0u, length + 1u};
        bytes[0x5000u] = initial_reference;
        return true;
    }
    void WriteTemporary(std::uint32_t destination,
                        std::string_view text) override {
        Check(destination == 0x5001u && text == result,
              "temporary receives formatted bytes");
        calls.emplace_back("write-temporary");
        for (std::uint32_t i = 0u; i < text.size(); ++i)
            bytes[destination + i] = static_cast<std::uint8_t>(text[i]);
    }
    std::uint8_t StackFlagByte() override {
        calls.emplace_back("flag");
        return 0x7du;
    }
    void ThrowRange() override { throw std::out_of_range("range"); }
    void ThrowLength() override { throw std::length_error("length"); }
    void DetachAndReset(pal4::StringBufferState&) override {
        Check(false, "distinct destination cannot self-assign");
    }
    bool AllocateText(pal4::StringBufferState&, std::uint32_t,
                      std::uint8_t) override {
        Check(false, "distinct destination needs no text allocation");
        return false;
    }
    void SetTextSize(pal4::StringBufferState&, std::uint32_t) override {
        Check(false, "distinct destination needs no text sizing");
    }
    void SetErrorFlag(pal4::StringBufferState&, std::uint8_t flag) override {
        Check(flag == 1u, "empty or unshareable source sets error flag");
        calls.emplace_back("set-error");
        if (fail_assign) throw std::runtime_error("assignment failed");
    }
    void Reallocate(pal4::StringBufferState& state,
                    std::uint32_t length) override {
        calls.emplace_back("reallocate");
        state = {0x6001u, 0u, length + 8u};
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read-header");
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.emplace_back("write-byte");
        bytes[address] = value;
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override {
        calls.emplace_back("copy");
        for (std::uint32_t i = 0u; i < length; ++i)
            bytes[destination + i] = bytes.at(source + i);
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x5000u, "temporary header free address");
        calls.emplace_back("free");
    }
};

void CheckSharedTemporary() {
    Runtime runtime;
    pal4::StringBuffer destination({0x9001u, 3u, 8u});
    std::uint8_t flag = 0u;
    Check(&pal4::TradeWindowStringFormatter::Format(
              destination, flag, 0x1000u, 0x2000u, runtime) ==
              &destination &&
              destination.State().data == 0x5001u &&
              destination.State().length == 8u &&
              flag == 0x7du &&
              runtime.bytes.at(0x5000u) == 0u &&
              runtime.formatted_with == 0x1000u &&
              runtime.arguments_at == 0x2000u,
          "formatted source is shared, then temporary reference released");
    Check(runtime.calls.front() == "format" &&
              runtime.calls[1] == "flag" &&
              runtime.calls[2] == "allocate" &&
              runtime.calls[3] == "write-temporary" &&
              runtime.calls.back() == "write-byte",
          "format, allocation, copy, assignment and cleanup order");
}

void CheckUnshareableAndEmpty() {
    Runtime runtime;
    runtime.initial_reference = 0xffu;
    pal4::StringBuffer destination;
    std::uint8_t flag = 0u;
    (void)pal4::TradeWindowStringFormatter::Format(
        destination, flag, 1u, 2u, runtime);
    Check(destination.State().data == 0x6001u &&
              destination.State().length == 8u &&
              runtime.bytes.at(0x6001u) == 'v' &&
              runtime.calls.back() == "free",
          "unshareable text is copied and temporary freed");

    Runtime failed;
    failed.allocate = false;
    failed.result = "ignored";
    pal4::StringBuffer empty;
    (void)pal4::TradeWindowStringFormatter::Format(
        empty, flag, 1u, 2u, failed);
    Check(empty.State().data == 0u && empty.State().length == 0u &&
              failed.calls.back() == "set-error",
          "allocation failure assigns empty source and has no header cleanup");
}

void CheckExceptionalCleanupAndCapacity() {
    Runtime runtime;
    runtime.initial_reference = 0xffu;
    runtime.fail_assign = true;
    pal4::StringBuffer destination;
    std::uint8_t flag = 0u;
    bool threw = false;
    try {
        (void)pal4::TradeWindowStringFormatter::Format(
            destination, flag, 1u, 2u, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls.back() == "free",
          "temporary released when assignment throws");

    Runtime oversized;
    oversized.result.assign(1024u, 'x');
    threw = false;
    try {
        (void)pal4::TradeWindowStringFormatter::Format(
            destination, flag, 1u, 2u, oversized);
    } catch (const std::length_error&) {
        threw = true;
    }
    Check(threw && oversized.calls == std::vector<std::string>{"format"},
          "oversized result cannot overwrite native-sized stack buffer");
}

}  // namespace

int main() {
    try {
        CheckSharedTemporary();
        CheckUnshareableAndEmpty();
        CheckExceptionalCleanupAndCapacity();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
