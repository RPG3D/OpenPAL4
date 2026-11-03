#include "pal4/pal_game_iv_string_records.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string action;
    std::uint32_t first;
    std::uint32_t second = 0;
    std::uint32_t third = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IPalGameIVStringRecordRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::uint32_t empty_reads = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write", address, value});
    }
    void Free(std::uint32_t address) override {
        calls.push_back({"free", address});
    }
    std::uint32_t Cleanup(std::uint32_t record) override {
        calls.push_back({"cleanup", record});
        return record + 1u;
    }
    std::uint32_t CopyConstruct(std::uint32_t destination,
                                std::uint32_t source) override {
        calls.push_back({"copy", destination, source});
        return destination;
    }
    std::uint32_t InitializeBuffer(std::uint32_t destination,
                                    std::uint32_t data,
                                    std::uint32_t length) override {
        calls.push_back({"init", destination, data, length});
        return destination + 9u;
    }
    std::uint32_t EmptyResult() override {
        ++empty_reads;
        return 0xdeadbeefu;
    }
};

void Cleanup() {
    using pal4::PalGameIVStringRecords;
    Runtime runtime;
    Check(PalGameIVStringRecords::CleanupRange(0x2000u, 0x2000u,
                                                runtime) == 0xdeadbeefu,
          "empty cleanup result");
    Check(runtime.calls.empty(), "empty cleanup must not touch records");
    Check(PalGameIVStringRecords::CleanupRange(0x2000u, 0x2018u,
                                                runtime) == 0x200du,
          "last cleanup return");
    Check(runtime.calls == std::vector<Call>({
              {"cleanup", 0x2000u}, {"cleanup", 0x200cu}}),
          "cleanup range order");
    Check(runtime.empty_reads == 1u,
          "populated cleanup does not request empty return");

    runtime.calls.clear();
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x1008u] = 0x2018u;
    runtime.words[0x100cu] = 0x2030u;
    Check(PalGameIVStringRecords::CleanupAndFree(0x1000u, runtime) == 0u,
          "owner cleanup returns zero");
    Check(runtime.calls == std::vector<Call>({
              {"cleanup", 0x2000u}, {"cleanup", 0x200cu},
              {"free", 0x2000u}, {"write", 0x1004u, 0u},
              {"write", 0x1008u, 0u}, {"write", 0x100cu, 0u}}),
          "owner cleanup order and fields");
    Check(runtime.empty_reads == 1u,
          "owner cleanup does not request empty return");
    runtime.calls.clear();
    Check(PalGameIVStringRecords::CleanupAndFree(0x1000u, runtime) == 0u &&
              runtime.calls.front() == Call{"free", 0u},
          "empty owner still frees begin");
    Check(runtime.empty_reads == 1u,
          "empty owner ignores unspecified register");
}

void Copy() {
    using pal4::PalGameIVStringRecords;
    Runtime runtime;
    Check(PalGameIVStringRecords::CopyRange(
              0x3000u, 0x3000u, 0x4000u, runtime) == 0x4000u &&
              runtime.calls.empty(), "empty copy retains destination");
    Check(PalGameIVStringRecords::CopyRange(
              0x3000u, 0x3018u, 0x4000u, runtime) == 0x4018u,
          "copy range returns end");
    Check(runtime.calls == std::vector<Call>({
              {"copy", 0x4000u, 0x3000u},
              {"copy", 0x400cu, 0x300cu}}), "copy range order");
    runtime.calls.clear();
    Check(PalGameIVStringRecords::CopyCount(
              0x5000u, 0u, 0x6000u, runtime) == 0xdeadbeefu &&
              runtime.calls.empty(), "empty count return");
    Check(PalGameIVStringRecords::CopyCount(
              0x5000u, 3u, 0x6000u, runtime) == 0x5018u,
          "count returns last constructor result");
    Check(runtime.calls == std::vector<Call>({
              {"copy", 0x5000u, 0x6000u},
              {"copy", 0x500cu, 0x6000u},
              {"copy", 0x5018u, 0x6000u}}),
          "count keeps source fixed");
    Check(runtime.empty_reads == 1u,
          "populated count does not request empty return");
}

void Initialize() {
    using pal4::PalGameIVStringRecords;
    Runtime runtime;
    runtime.words[0x7000u] = 0xabc0u;
    runtime.words[0x7004u] = 5u;
    Check(PalGameIVStringRecords::InitializeRange(
              0x8000u, 0x8000u, 0x7000u, runtime) == 0xdeadbeefu &&
              runtime.calls.empty(), "empty forward init");
    Check(PalGameIVStringRecords::InitializeRange(
              0x8000u, 0x8018u, 0x7000u, runtime) == 0x8015u,
          "forward init final result");
    Check(runtime.calls == std::vector<Call>({
              {"init", 0x8000u, 0xabc0u, 5u},
              {"init", 0x800cu, 0xabc0u, 5u}}),
          "forward init repeats descriptor");
    Check(runtime.empty_reads == 1u,
          "populated init does not request empty return");
    runtime.calls.clear();
    runtime.words[0x9000u] = 0xaaaau;
    runtime.words[0x9004u] = 1u;
    runtime.words[0x900cu] = 0xbbbbu;
    runtime.words[0x9010u] = 2u;
    Check(PalGameIVStringRecords::InitializeReverse(
              0x9000u, 0x9000u, 0xa018u, runtime) == 0xa018u &&
              runtime.calls.empty(), "empty reverse init");
    Check(PalGameIVStringRecords::InitializeReverse(
              0x9000u, 0x9018u, 0xa018u, runtime) == 0xa000u,
          "reverse init returns destination begin");
    Check(runtime.calls == std::vector<Call>({
              {"init", 0xa00cu, 0xbbbbu, 2u},
              {"init", 0xa000u, 0xaaaau, 1u}}),
          "reverse init order and source fields");
}

void InvalidRanges() {
    using pal4::PalGameIVStringRecords;
    Runtime runtime;
    for (const auto [begin, end] : {
             std::pair{0x1000u, 0x1001u},
             std::pair{0x100cu, 0x1000u}}) {
        try {
            (void)PalGameIVStringRecords::CleanupRange(begin, end, runtime);
            Check(false, "invalid range must fail");
        } catch (const std::invalid_argument&) {
            Check(runtime.calls.empty(), "invalid range leaves runtime intact");
        }
        try {
            (void)PalGameIVStringRecords::CopyRange(
                begin, end, 0x2000u, runtime);
            Check(false, "invalid copy range must fail");
        } catch (const std::invalid_argument&) {
            Check(runtime.calls.empty(), "invalid copy leaves runtime intact");
        }
    }
}

}  // namespace

int main() {
    try {
        Cleanup();
        Copy();
        Initialize();
        InvalidRanges();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
