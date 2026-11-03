#include "pal4/combat_event_logger.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatLogFileRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t opened_handle = 0x4000u;
    int close_result = 0;

    Runtime() {
        words[0x1000u + 4u] = 0x8480d0u;
        words[0x8480d4u] = 88u;
        words[0x1000u + 88u] = 0u;
        words[0x1000u + 148u] = 0u;
        words[0x1000u + 152u] = 0u;
        bytes[0x1000u + 96u] = 0u;
        bytes[0x1000u + 292u] = 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read-byte:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("write-byte:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    int CloseFile(std::uint32_t handle) override {
        calls.push_back("close:" + std::to_string(handle));
        return close_result;
    }
    void InitializeFileBuffer(std::uint32_t buffer,
                              std::uint32_t handle,
                              std::uint32_t mode) override {
        Check(buffer == 0x1000u + 8u, "file buffer address");
        calls.push_back("init:" + std::to_string(handle) + ":" +
                        std::to_string(mode));
        words[0x1000u + 88u] = handle;
    }
    void SetIoStreamState(std::uint32_t ios, std::uint32_t state,
                          bool raise) override {
        Check(ios == 0x1000u + 92u && state == 2u && !raise,
              "badbit setter arguments");
        calls.emplace_back("badbit");
        bytes[0x1000u + 96u] |= 2u;
    }
    std::uint32_t OpenFile(std::string_view filename,
                           std::uint32_t mode) override {
        Check(filename == "combat.log", "file name forwarded");
        calls.push_back("open:" + std::to_string(mode));
        return opened_handle;
    }
    void GetCodecvtFacet(std::uint32_t buffer) override {
        Check(buffer == 0x1000u + 8u, "codecvt buffer address");
        calls.emplace_back("facet");
    }
    void InsertPrimary(std::uint32_t stream,
                       std::uint32_t text) override {
        Check(stream == 0x1000u + 4u, "replay stream address");
        calls.push_back("insert:" + std::to_string(text));
    }
    void FlushPrimary(std::uint32_t stream) override {
        Check(stream == 0x1000u + 4u, "replay flush address");
        calls.emplace_back("flush");
    }
    void ReleaseText(std::uint32_t record) override {
        calls.push_back("release:" + std::to_string(record));
    }
};

std::size_t FindCall(const Runtime& runtime, const std::string& call) {
    for (std::size_t i = 0u; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing call: " + call);
}

void FreshOpenModes() {
    for (const auto [argument, expected_mode] : {
             std::pair{0u, 18u}, std::pair{1u, 10u},
             std::pair{0x100u, 18u}}) {
        Runtime runtime;
        const auto result = pal4::CombatEventLogger::SetLogFile(
            0x1000u, "combat.log", argument, runtime);
        Check(result == 0x1000u &&
                  runtime.words[0x1000u + 88u] == 0x4000u &&
                  runtime.words[0x1000u + 112u] == 2u,
              "fresh open initializes buffer and ios format word");
        Check(FindCall(runtime, "open:" + std::to_string(expected_mode)) <
                  FindCall(runtime, "init:16384:1") &&
                  FindCall(runtime, "init:16384:1") <
                  FindCall(runtime, "facet"),
              "open mode and codecvt order");
    }
}

void ReplaceExistingFile() {
    Runtime runtime;
    runtime.words[0x1000u + 88u] = 0x3000u;
    const auto result = pal4::CombatEventLogger::SetLogFile(
        0x1000u, "combat.log", 0u, runtime);
    Check(result == 0x1000u, "replacement return value");
    Check(FindCall(runtime, "close:12288") <
              FindCall(runtime, "init:0:2") &&
              FindCall(runtime, "init:0:2") <
              FindCall(runtime, "open:18"),
          "existing file closed before new file opens");
}

void OpenFailures() {
    for (const auto old_handle : {0u, 0x3000u}) {
        Runtime runtime;
        runtime.words[0x1000u + 88u] = old_handle;
        if (old_handle != 0u) runtime.close_result = -1;
        else runtime.opened_handle = 0u;
        try {
            static_cast<void>(pal4::CombatEventLogger::SetLogFile(
                0x1000u, "combat.log", 0u, runtime));
            throw std::runtime_error("expected open failure");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) ==
                      "skLogger::setLogFilename - Failed to open file.",
                  "IDA exception message");
        }
        Check(FindCall(runtime, "badbit") <
                  FindCall(runtime, "read-byte:4192"),
              "badbit checked before replay");
        Check(runtime.words.find(0x1000u + 112u) == runtime.words.end() &&
                  runtime.bytes[0x1000u + 292u] == 0u,
              "failed open leaves format and collection untouched");
    }
}

void ReplayBufferedEvents() {
    Runtime runtime;
    runtime.words[0x1000u] = 2u;
    runtime.bytes[0x1000u + 292u] = 1u;
    runtime.words[0x1000u + 148u] = 0x2000u;
    runtime.words[0x1000u + 152u] = 0x203cu;
    runtime.words[0x2000u + 4u] = 0x5000u;
    runtime.words[0x2000u + 16u] = 0u;
    runtime.words[0x2014u + 4u] = 0x5100u;
    runtime.words[0x2014u + 16u] = 3u;
    runtime.words[0x2028u + 4u] = 0u;
    runtime.words[0x2028u + 16u] = 1u;

    const auto result = pal4::CombatEventLogger::SetLogFile(
        0x1000u, "combat.log", 1u, runtime);
    Check(result == 0x203cu &&
              runtime.bytes[0x1000u + 292u] == 0u &&
              runtime.words[0x1000u + 152u] == 0x2000u,
          "replay returns old end and clears buffered collection");
    Check(FindCall(runtime, "write-byte:4388:0") <
              FindCall(runtime, "insert:20480") &&
              FindCall(runtime, "insert:20480") <
              FindCall(runtime, "insert:8657252") &&
              FindCall(runtime, "insert:8657252") <
              FindCall(runtime, "release:8192") &&
              FindCall(runtime, "release:8192") <
              FindCall(runtime, "release:8212") &&
              FindCall(runtime, "release:8212") <
              FindCall(runtime, "release:8232") &&
              FindCall(runtime, "release:8232") <
              FindCall(runtime, "write:4248:8192"),
          "thresholded replay and release order");
    Check(FindCall(runtime, "flush") <
              FindCall(runtime, "release:8192"),
          "replayed entries flushed before release");
    for (const auto& call : runtime.calls)
        Check(call != "insert:20736", "over-threshold entry is skipped");
}

}  // namespace

int main() {
    try {
        FreshOpenModes();
        ReplaceExistingFile();
        OpenFailures();
        ReplayBufferedEvents();
        std::cout << "combat log file tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
