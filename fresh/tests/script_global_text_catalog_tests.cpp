#include "pal4/script_global_text_catalog.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Event {
    std::string kind;
    std::uint32_t address = 0u;
    std::uint32_t value = 0u;
    std::string text;
};

struct Runtime final : pal4::IScriptGlobalTextRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, bool> allocation_success;
    std::map<std::uint32_t, std::uint32_t> allocation_pointer;
    std::vector<Event> events;
    int registration_result = -17;
    std::uint32_t initialized_pointer = 0x1234u;
    std::uint32_t throw_on_message = 0u;

    std::uint8_t ReadByte(std::uint32_t address) override {
        events.push_back({"read_byte", address});
        return bytes.at(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back({"read_word", address});
        return words.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        events.push_back({"write_byte", address, value});
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        events.push_back({"write_word", address, value});
        words[address] = value;
    }
    void SetErrorFlag(std::uint32_t record, std::uint8_t value) override {
        events.push_back({"error_flag", record, value});
    }
    void SetExceptionMessage(std::uint32_t record,
                             std::string_view text) override {
        events.push_back({"message", record, 0u, std::string(text)});
        if (record == throw_on_message)
            throw std::runtime_error("message assignment failed");
    }
    std::uint32_t InitializeCString(std::uint32_t record,
                                    std::string_view text,
                                    std::uint8_t seed) override {
        events.push_back({"cstring", record, seed, std::string(text)});
        return initialized_pointer;
    }
    std::uint8_t AllocateBuffer(std::uint32_t record,
                                std::uint32_t length,
                                std::uint8_t mode) override {
        Check(mode == 1u, "allocation mode");
        events.push_back({"allocate", record, length});
        if (!allocation_success.at(record)) return 0u;
        words[record + 4u] = allocation_pointer.at(record);
        return 1u;
    }
    void CopyBytes(std::uint32_t destination,
                   std::string_view text) override {
        events.push_back({"copy", destination, 0u, std::string(text)});
    }
    int RegisterAtExit(std::uint32_t callback) override {
        events.push_back({"atexit", callback});
        return registration_result;
    }
    void DestroyRange(std::uint32_t first, std::uint32_t stride,
                      std::uint32_t count,
                      std::uint32_t destructor) override {
        Check(stride == 16u && destructor == 0x51f9b0u,
              "vector destructor layout");
        events.push_back({"destroy", first, count});
    }
};

void CheckEvent(const Event& actual, std::string_view kind,
                std::uint32_t address, std::uint32_t value,
                std::string_view text, const char* message) {
    Check(actual.kind == kind && actual.address == address &&
              actual.value == value && actual.text == text, message);
}

void PrimaryInitialization() {
    constexpr std::array<std::string_view, 16> texts{{
        "\xb5\xc8\xbc\xb6", "\xcb\xc0\xcd\xf6", "\xc9\xf1", "\xbe\xab",
        "\xc6\xf8", "\xc6\xf8", "\xc6\xd5\xcd\xa8\xb9\xa5\xbb\xf7",
        "\xb1\xa9\xbb\xf7\xb9\xa5\xbb\xf7", "\xb8\xf1\xb5\xb5",
        "\xb9\xa5\xbb\xf7\xce\xb4\xc3\xfc\xd6\xd0", "\xb6\xe3\xb1\xdc",
        "\xd4\xae\xbb\xa4", "\xba\xcf\xbb\xf7", "\xb7\xc0\xd3\xf9",
        "\xcc\xd3\xc5\xdc", "\xbe\xad\xd1\xe9",
    }};
    Runtime r;
    Check(pal4::ScriptGlobalTextCatalog::InitializePrimary(0x5au, r) ==
              0x1234u && r.events.size() == 49u,
          "primary return and sixteen records plus final CString");
    for (std::uint32_t i = 0; i < texts.size(); ++i) {
        const auto address = 0x8fa0b8u + i * 16u;
        CheckEvent(r.events[i * 3u], "write_byte", address, 0x5au, "",
                   "primary seed address and order");
        CheckEvent(r.events[i * 3u + 1u], "error_flag", address, 0u, "",
                   "primary reset order");
        CheckEvent(r.events[i * 3u + 2u], "message", address, 0u, texts[i],
                   "primary GBK literal and assignment order");
    }
    CheckEvent(r.events.back(), "cstring", 0x8fa1b8u, 0x5au, "Max",
               "primary seventeenth CString");

    r.events.clear();
    Check(pal4::ScriptGlobalTextCatalog::RegisterPrimary(0x7fu, r) == -17 &&
              r.events.size() == 50u,
          "primary registration forwards CRT result");
    CheckEvent(r.events.back(), "atexit", 0x56efe0u, 0u, "",
               "primary cleanup registered after initialization");

    r.events.clear();
    r.throw_on_message = 0x8fa0c8u;
    bool threw = false;
    try {
        (void)pal4::ScriptGlobalTextCatalog::RegisterPrimary(0x11u, r);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && r.events.size() == 6u &&
              r.events.back().kind == "message",
          "failed primary assignment prevents later records and registration");
}

void SecondaryInitialization() {
    constexpr std::array<std::string_view, 3> direct{{
        "\xbd\xf0\xc7\xae", "\xc6\xc0\xb7\xd6", "\xba\xc4\xca\xb1"}};
    Runtime r;
    r.allocation_success[0x8fa090u] = true;
    r.allocation_success[0x8fa0a0u] = true;
    r.allocation_pointer[0x8fa090u] = 0x3011u;
    r.allocation_pointer[0x8fa0a0u] = 0x4012u;
    Check(pal4::ScriptGlobalTextCatalog::InitializeSecondary(0x66u, r) ==
              0x12u,
          "secondary returns low byte of final successful buffer pointer");
    for (std::uint32_t i = 0; i < direct.size(); ++i) {
        const auto address = 0x8fa060u + i * 16u;
        const auto& first = r.events[i * 5u];
        CheckEvent(first, "write_byte", address, 0x66u, "",
                   "secondary direct seed");
        for (std::uint32_t j = 0; j < 3u; ++j)
            CheckEvent(r.events[i * 5u + j + 1u], "write_word",
                       address + 4u * (j + 1u), 0u, "",
                       "secondary direct zeroed word");
        CheckEvent(r.events[i * 5u + 4u], "message", address, 0u,
                   direct[i], "secondary direct GBK text");
    }
    constexpr std::array<std::string_view, 2> allocated{{
        "\xd5\xbd\xb6\xb7\xbd\xe1\xb9\xfb", "Max"}};
    constexpr std::array<std::uint32_t, 2> records{{0x8fa090u, 0x8fa0a0u}};
    constexpr std::array<std::uint32_t, 2> buffers{{0x3011u, 0x4012u}};
    Check(r.events.size() == 29u, "secondary exact allocation event count");
    for (std::uint32_t i = 0; i < 2u; ++i) {
        const auto base = 15u + i * 7u;
        CheckEvent(r.events[base], "write_byte", records[i], 0x66u, "",
                   "secondary allocated seed");
        CheckEvent(r.events[base + 1u], "error_flag", records[i], 0u, "",
                   "secondary allocated reset");
        CheckEvent(r.events[base + 2u], "allocate", records[i],
                   static_cast<std::uint32_t>(allocated[i].size()), "",
                   "secondary allocation length");
        CheckEvent(r.events[base + 3u], "read_word", records[i] + 4u, 0u,
                   "", "secondary allocation pointer read");
        CheckEvent(r.events[base + 4u], "copy", buffers[i], 0u,
                   allocated[i], "secondary exact GBK bytes copied");
        CheckEvent(r.events[base + 5u], "write_word", records[i] + 8u,
                   static_cast<std::uint32_t>(allocated[i].size()), "",
                   "secondary text length stored");
        CheckEvent(r.events[base + 6u], "write_byte",
                   buffers[i] + static_cast<std::uint32_t>(allocated[i].size()),
                   0u, "", "secondary text terminated");
    }

    r.events.clear();
    Check(pal4::ScriptGlobalTextCatalog::RegisterSecondary(0x10u, r) ==
              -17 && r.events.back().kind == "atexit" &&
              r.events.back().address == 0x56f1b0u,
          "secondary registration follows initialization");

    r = Runtime{};
    r.allocation_success[0x8fa090u] = false;
    r.allocation_success[0x8fa0a0u] = false;
    Check(pal4::ScriptGlobalTextCatalog::InitializeSecondary(0u, r) == 0u &&
              r.events.size() == 21u,
          "both allocation failures skip copy and return zero");
    CheckEvent(r.events[18], "write_byte", 0x8fa0a0u, 0u, "",
               "second allocation still attempted after first failure");
    r = Runtime{};
    r.allocation_success[0x8fa090u] = false;
    r.allocation_success[0x8fa0a0u] = true;
    r.allocation_pointer[0x8fa0a0u] = 0x4012u;
    Check(pal4::ScriptGlobalTextCatalog::InitializeSecondary(0u, r) ==
              0x12u && r.events.size() == 25u,
          "second allocation succeeds after first failure and supplies result");
    r = Runtime{};
    r.allocation_success[0x8fa090u] = true;
    r.allocation_success[0x8fa0a0u] = false;
    r.allocation_pointer[0x8fa090u] = 0x3011u;
    Check(pal4::ScriptGlobalTextCatalog::InitializeSecondary(0u, r) == 0u &&
              r.events.size() == 25u,
          "final allocation failure determines the returned zero byte");
}

void CleanupGuards() {
    Runtime r;
    r.bytes[0x8fa0b0u] = 0x80u;
    r.bytes[0x8fa058u] = 0x40u;
    pal4::ScriptGlobalTextCatalog::CleanupPrimary(r);
    pal4::ScriptGlobalTextCatalog::CleanupSecondary(r);
    Check(r.events.size() == 6u, "each first cleanup reads, sets and destroys");
    CheckEvent(r.events[0], "read_byte", 0x8fa0b0u, 0u, "",
               "primary guard read");
    CheckEvent(r.events[1], "write_byte", 0x8fa0b0u, 0x81u, "",
               "primary guard set before destruction");
    CheckEvent(r.events[2], "destroy", 0x8fa0b8u, 17u, "",
               "primary 17 records destroyed");
    CheckEvent(r.events[3], "read_byte", 0x8fa058u, 0u, "",
               "secondary guard read");
    CheckEvent(r.events[4], "write_byte", 0x8fa058u, 0x41u, "",
               "secondary guard set before destruction");
    CheckEvent(r.events[5], "destroy", 0x8fa060u, 5u, "",
               "secondary five records destroyed");
    pal4::ScriptGlobalTextCatalog::CleanupPrimary(r);
    pal4::ScriptGlobalTextCatalog::CleanupSecondary(r);
    Check(r.events.size() == 8u && r.events[6].kind == "read_byte" &&
              r.events[7].kind == "read_byte",
          "repeated cleanup only reads each guard");
}

}  // namespace

int main() {
    try {
        PrimaryInitialization();
        SecondaryInitialization();
        CleanupGuards();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
