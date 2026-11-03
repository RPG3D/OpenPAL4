#include "pal4/file_enumerator.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IFileEnumeratorRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t pattern = 0x9000u;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t first_handle = 0xabcdu;
    std::uint32_t close_result = 7u;
    std::uint32_t next_result = 1u;
    bool full_path_ok = true;
    std::string full_path = "C:\\base/sub/*.txt";
    std::string found_name = "entry.txt";

    Runtime() {
        PutString(0x8c26bcu, "*.*");
        PutString(0x8dc780u, "");
        PutString(pattern, "sub/*.txt");
    }
    void PutString(const std::uint32_t address, const std::string& value) {
        for (std::size_t index = 0; index < value.size(); ++index)
            bytes[address + static_cast<std::uint32_t>(index)] =
                static_cast<std::uint8_t>(value[index]);
        bytes[address + static_cast<std::uint32_t>(value.size())] = 0u;
    }
    [[nodiscard]] std::string StringAt(const std::uint32_t address) {
        std::string result;
        for (std::uint32_t index = 0u; index < 1024u; ++index) {
            const auto byte = bytes[address + index];
            if (byte == 0u) break;
            result.push_back(static_cast<char>(byte));
        }
        return result;
    }
    [[nodiscard]] std::uint8_t ReadByte(
        const std::uint32_t address) override {
        return bytes[address];
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        return words[address];
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
        events.emplace_back("write-byte:" + std::to_string(address));
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("write-word:" + std::to_string(address));
    }
    void CopyString(const std::uint32_t destination,
                    const std::uint32_t source) override {
        const auto value = StringAt(source);
        PutString(destination, value);
        events.emplace_back("copy:" + std::to_string(destination));
    }
    [[nodiscard]] std::uint32_t FindFirst(
        const std::uint32_t value,
        const std::uint32_t find_data) override {
        Check(find_data == object + 324u &&
                  (value == pattern || value == 0x8c26bcu),
              "FindFirst receives pattern and embedded WIN32_FIND_DATA");
        events.emplace_back("find-first");
        PutString(find_data + 44u, found_name);
        bytes[find_data] = 0x10u;
        return first_handle;
    }
    [[nodiscard]] std::uint32_t FindNext(
        const std::uint32_t handle,
        const std::uint32_t find_data) override {
        Check(find_data == object + 324u && handle == words[object + 644u],
              "FindNext uses stored handle and embedded find data");
        events.emplace_back("find-next");
        return next_result;
    }
    [[nodiscard]] std::uint32_t FindClose(
        const std::uint32_t handle) override {
        Check(handle == words[object + 644u], "FindClose uses stored handle");
        events.emplace_back("find-close");
        return close_result;
    }
    [[nodiscard]] bool FullPath(const std::uint32_t destination,
                                const std::uint32_t value,
                                const std::uint32_t capacity) override {
        Check(destination == object && capacity == 324u &&
                  (value == pattern || value == 0x8c26bcu),
              "fullpath uses object base and 324-byte capacity");
        events.emplace_back("fullpath");
        if (full_path_ok) PutString(destination, full_path);
        return full_path_ok;
    }
    [[nodiscard]] std::uint32_t FindLastCharacter(
        const std::uint32_t string,
        const std::uint8_t character) override {
        Check(string == object &&
                  (character == '\\' || character == '/'),
              "searches the full path for both separator styles");
        events.emplace_back("find-last");
        std::uint32_t result = 0u;
        const auto value = StringAt(string);
        for (std::size_t index = 0; index < value.size(); ++index)
            if (static_cast<std::uint8_t>(value[index]) == character)
                result = string + static_cast<std::uint32_t>(index);
        return result;
    }
};

}  // namespace

int main() {
    using Enumerator = pal4::FileEnumerator;
    Runtime runtime;
    runtime.PutString(Runtime::object, "old");
    runtime.PutString(Runtime::object + 648u, "old-name");
    runtime.PutString(Runtime::object + 972u, "old-other");
    runtime.bytes[Runtime::object + 1296u] = 1u;
    Check(Enumerator::ClearStrings(Runtime::object, runtime) ==
              Runtime::object &&
              runtime.StringAt(Runtime::object).empty() &&
              runtime.StringAt(Runtime::object + 648u).empty() &&
              runtime.StringAt(Runtime::object + 972u).empty() &&
              runtime.bytes[Runtime::object + 1296u] == 0u &&
              runtime.events.size() == 4u,
          "clear initializes three string regions and enumerator flag");
    runtime.events.clear();
    runtime.words[Runtime::object + 644u] = 0u;
    Check(Enumerator::Close(Runtime::object, runtime) == 0u &&
              runtime.bytes[Runtime::object + 1296u] == 1u &&
              runtime.events.size() == 1u,
          "zero handle sets done flag without FindClose");
    runtime.events.clear();
    runtime.words[Runtime::object + 644u] = 0xffffffffu;
    Check(Enumerator::Close(Runtime::object, runtime) == 0xffffffffu &&
              runtime.words[Runtime::object + 644u] == 0xffffffffu &&
              runtime.events.size() == 1u,
          "invalid handle remains stored and is not closed");
    runtime.events.clear();
    runtime.words[Runtime::object + 644u] = 0xabcdu;
    Check(Enumerator::Close(Runtime::object, runtime) == 7u &&
              runtime.words[Runtime::object + 644u] == 0u &&
              runtime.events == std::vector<std::string>({
                  "find-close", "write-word:" +
                      std::to_string(Runtime::object + 644u),
                  "write-byte:" +
                      std::to_string(Runtime::object + 1296u)}),
          "valid close forwards FindClose and clears handle before done flag");
    runtime.events.clear();
    runtime.bytes[Runtime::object + 1296u] = 5u;
    Check(Enumerator::IsDone(Runtime::object, runtime) == 5u &&
              runtime.events.empty(),
          "nonzero done byte is returned without closing");
    runtime.bytes[Runtime::object + 1296u] = 0u;
    runtime.words[Runtime::object + 644u] = 0xabcdu;
    Check(Enumerator::IsDone(Runtime::object, runtime) == 7u &&
              runtime.bytes[Runtime::object + 1296u] == 1u,
          "zero done byte tail-calls close and returns low result byte");
    runtime.events.clear();
    Check(Enumerator::Initialize(Runtime::object, Runtime::pattern,
                                  runtime) == 1u &&
              runtime.words[Runtime::object + 644u] == 0xabcdu &&
              runtime.bytes[Runtime::object + 1296u] == 0u &&
              runtime.StringAt(Runtime::object) == "C:\\base/sub" &&
              runtime.StringAt(Runtime::object + 368u) == "entry.txt" &&
              runtime.events[0] == "write-byte:" +
                  std::to_string(Runtime::object + 1296u) &&
              runtime.events[2] == "copy:" +
                  std::to_string(Runtime::object + 368u),
          "initialize closes prior state, searches, and truncates later separator");
    runtime.events.clear();
    Check(Enumerator::Next(Runtime::object, runtime) == 1u &&
              runtime.events == std::vector<std::string>({"find-next"}),
          "next forwards active search handle");
    runtime.events.clear();
    runtime.words[Runtime::object + 644u] = 0u;
    Check(Enumerator::Next(Runtime::object, runtime) == 0u &&
              runtime.events.empty(), "next skips zero handle");
    runtime.words[Runtime::object + 644u] = 0xffffffffu;
    Check(Enumerator::Next(Runtime::object, runtime) == 1u,
          "next forwards nonzero invalid sentinel as native code does");
    runtime.words[Runtime::object + 644u] = 0xabcdu;
    runtime.PutString(Runtime::object + 368u, ".");
    Check(Enumerator::IsDirectory(Runtime::object, runtime) &&
              Enumerator::IsSpecialDirectory(Runtime::object, runtime),
          "dot directory is recognized");
    runtime.PutString(Runtime::object + 368u, "..");
    Check(Enumerator::IsSpecialDirectory(Runtime::object, runtime),
          "dot-dot directory is recognized");
    runtime.PutString(Runtime::object + 368u, "...");
    Check(!Enumerator::IsSpecialDirectory(Runtime::object, runtime),
          "three dots are not a special directory");
    runtime.bytes[Runtime::object + 324u] = 0u;
    runtime.PutString(Runtime::object + 368u, ".");
    Check(!Enumerator::IsDirectory(Runtime::object, runtime) &&
              !Enumerator::IsSpecialDirectory(Runtime::object, runtime),
          "directory flag is required for special names");
    runtime.bytes[Runtime::object + 324u] = 0x10u;
    runtime.words[Runtime::object + 644u] = 0u;
    Check(!Enumerator::IsSpecialDirectory(Runtime::object, runtime),
          "special-directory query requires nonzero handle");
    runtime.PutString(Runtime::object + 368u, "current.txt");
    Check(Enumerator::GetCurrentFileName(Runtime::object, runtime) ==
              Runtime::object + 648u &&
              runtime.StringAt(Runtime::object + 648u) == "current.txt",
          "current filename copies from find data to secondary buffer");
    runtime.events.clear();
    runtime.first_handle = 0xffffffffu;
    Check(Enumerator::Initialize(Runtime::object, 0u, runtime) == 0u &&
              runtime.bytes[Runtime::object + 1296u] == 1u &&
              runtime.words[Runtime::object + 644u] == 0xffffffffu,
          "default pattern and failed FindFirst close enumeration");
    runtime.events.clear();
    runtime.first_handle = 0xabcdu;
    runtime.full_path_ok = false;
    Check(Enumerator::Initialize(Runtime::object, Runtime::pattern,
                                  runtime) == 0u &&
              runtime.words[Runtime::object + 644u] == 0u &&
              runtime.bytes[Runtime::object + 1296u] == 1u,
          "fullpath failure closes opened handle");
    runtime.events.clear();
    runtime.full_path_ok = true;
    runtime.full_path = "plain";
    Check(Enumerator::Initialize(Runtime::object, Runtime::pattern,
                                  runtime) == 1u &&
              runtime.StringAt(Runtime::object) == "plain",
          "path without separators remains unchanged");
    pal4::FileEnumeratorRuntimeStub stub;
    Check(Enumerator::Initialize(1u, 0u, stub) == 0u,
          "Win32 and CRT boundary stub links");
    return EXIT_SUCCESS;
}
