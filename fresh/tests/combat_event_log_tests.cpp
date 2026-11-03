#include "pal4/combat_event_logger.h"

#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEventLogRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, std::string> strings;
    std::optional<pal4::CombatLogTime> local_time{
        pal4::CombatLogTime{6u, 5u, 4u, 3u, 10u, 124u}};
    std::vector<std::string> calls;
    std::string buffer_text;
    std::uint32_t recorded_level = 0xffffffffu;
    std::uint32_t recorded_position = 0u;
    std::string primary_text;

    Runtime() {
        words[0x1000u] = 3u;
        words[0x1000u + 152u] = 0x3000u;
        bytes[0x1000u + 292u] = 1u;
        strings[0x2000u] = "battle ready";
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("word:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("byte:" + std::to_string(address));
        return bytes.at(address);
    }
    void FormatMessage(std::uint32_t destination, const char* format,
                       std::va_list arguments) override {
        char buffer[256]{};
        const auto length = std::vsnprintf(buffer, sizeof(buffer), format,
                                           arguments);
        Check(length >= 0 && length < static_cast<int>(sizeof(buffer)),
              "test formatted message fits buffer");
        strings[destination] = buffer;
        calls.push_back("format:" + std::to_string(destination));
    }
    std::string ReadCString(std::uint32_t address) override {
        calls.push_back("string:" + std::to_string(address));
        return strings.at(address);
    }
    std::optional<pal4::CombatLogTime> LocalTime() override {
        calls.emplace_back("time");
        return local_time;
    }
    void ResetBuffer(std::uint32_t buffer) override {
        calls.push_back("reset:" + std::to_string(buffer));
        buffer_text.clear();
    }
    void WriteBuffer(std::uint32_t buffer, std::string_view text) override {
        calls.push_back("write-buffer:" + std::to_string(buffer));
        buffer_text.append(text);
    }
    void FlushBuffer(std::uint32_t buffer) override {
        calls.push_back("flush-buffer:" + std::to_string(buffer));
    }
    std::uint8_t RecordEvent(std::uint32_t logger,
                             std::uint32_t insertion_point,
                             std::uint32_t level,
                             std::string_view text) override {
        Check(logger == 0x1000u && text == buffer_text,
              "record uses current buffer text");
        calls.emplace_back("record");
        recorded_level = level;
        recorded_position = insertion_point;
        return 0x5au;
    }
    void WritePrimary(std::uint32_t stream,
                      std::string_view text) override {
        Check(stream == 0x1000u + 4u, "primary stream address");
        calls.emplace_back("write-primary");
        primary_text = text;
    }
    std::uint32_t FlushPrimary(std::uint32_t stream) override {
        Check(stream == 0x1000u + 4u, "primary flush address");
        calls.emplace_back("flush-primary");
        return 0x8480d0u;
    }
};

void ThresholdAndTimeFailure() {
    Runtime runtime;
    runtime.words[0x1000u] = 1u;
    Check(pal4::CombatEventLogger::Log(0x1000u, 2u, 0x2000u,
                                       runtime, "value %d", 7) == 1u,
          "suppressed log returns threshold");
    Check(runtime.calls == std::vector<std::string>{"word:4096"},
          "suppressed log has no side effects");

    runtime.words[0x1000u] = 3u;
    runtime.local_time.reset();
    runtime.calls.clear();
    Check(pal4::CombatEventLogger::Log(0x1000u, 2u, 0x2000u,
                                       runtime, "value %d", 7) == 0u,
          "localtime failure returns null");
    Check(runtime.calls == std::vector<std::string>{
              "word:4096", "format:4389", "time"},
          "format happens before failed time lookup");
    Check(runtime.strings[0x1000u + 293u] == "value 7",
          "formatted message stored at logger offset 293");
}

void CollectedLabelsAndDate() {
    constexpr const char* kLabels[] = {
        "(Error)\t", "(InfL1)\t", "(InfL2)\t", "(InfL3)\t",
        "(Unkwn)\t"};
    for (std::uint32_t level = 0u; level < 5u; ++level) {
        Runtime runtime;
        runtime.words[0x1000u] = 5u;
        Check(pal4::CombatEventLogger::Log(0x1000u, level, 0x2000u,
                                           runtime) == 0x5au,
              "collection return is record cleanup result");
        const auto expected = std::string("03/11/2024 04:05:06 ") +
                              kLabels[level] + "battle ready\n";
        Check(runtime.buffer_text == expected,
              "date, time, label, message and newline");
        Check(runtime.recorded_level == level &&
                  runtime.recorded_position == 0x3000u,
              "record appended at collection end");
        Check(runtime.calls == std::vector<std::string>{
                  "word:4096", "time", "reset:4260", "string:8192",
                  "write-buffer:4260", "flush-buffer:4260", "byte:4388",
                  "word:4248", "record"},
              "collection branch order");
    }
}

void FormattedPrimaryOutput() {
    Runtime runtime;
    runtime.bytes[0x1000u + 292u] = 0u;
    Check(pal4::CombatEventLogger::Log(0x1000u, 1u, 0x2000u,
                                       runtime, "hit %d / %s", 7,
                                       "fire") == 0x8480d0u,
          "file branch returns stream value");
    Check(runtime.buffer_text ==
              "03/11/2024 04:05:06 (InfL1)\tbattle ready\thit 7 / fire\n" &&
              runtime.primary_text == runtime.buffer_text,
          "formatted text is tab-separated and written to primary stream");
    Check(runtime.calls == std::vector<std::string>{
              "word:4096", "format:4389", "time", "reset:4260",
              "string:8192", "string:4389", "write-buffer:4260",
              "flush-buffer:4260", "byte:4388", "write-primary",
              "flush-primary"},
          "formatted file branch order");
}

}  // namespace

int main() {
    try {
        ThresholdAndTimeFailure();
        CollectedLabelsAndDate();
        FormattedPrimaryOutput();
        std::cout << "combat event log tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
