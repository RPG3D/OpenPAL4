#include "pal4/combat_event_data_processor.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEventDataRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> matches;
    std::vector<std::uint32_t> count_schedule;
    std::size_t count_index = 0u;
    std::int32_t lower = 5;
    std::int32_t upper = 10;
    std::uint8_t collection_key = 0u;
    int database_reads = 0;
    int count_reads = 0;
    int base_reads = 0;
    int free_calls = 0;
    int cleanup_calls = 0;
    int validation_calls = 0;
    bool throw_on_insert = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        if (address == 0x5000u + 96u) ++base_reads;
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        ++database_reads;
        return 0x4000u;
    }
    std::uint32_t InitializeMatches(std::uint8_t key_low_byte) override {
        calls.emplace_back("init");
        collection_key = key_low_byte;
        return 0x7000u;
    }
    std::uint32_t UpgradeRecordCount(std::uint32_t holder) override {
        Check(holder == 0x5000u + 92u, "upgrade count holder");
        calls.emplace_back("count");
        ++count_reads;
        if (count_schedule.empty()) return 0u;
        const auto index = std::min(count_index, count_schedule.size() - 1u);
        ++count_index;
        return count_schedule[index];
    }
    void InsertMatch(std::uint32_t collection,
                     std::uint32_t entry) override {
        Check(collection == 0x7000u, "match collection handle");
        calls.emplace_back("insert");
        if (throw_on_insert) throw std::runtime_error("insert failure");
        matches.push_back(entry);
    }
    std::uint32_t MatchCount(std::uint32_t collection) override {
        Check(collection == 0x7000u, "match count handle");
        calls.emplace_back("match-count");
        return static_cast<std::uint32_t>(matches.size());
    }
    std::uint32_t FirstMatch(std::uint32_t collection) override {
        Check(collection == 0x7000u && !matches.empty(),
              "first match handle");
        calls.emplace_back("first");
        return matches.front();
    }
    std::int32_t LowerBound(std::uint32_t table,
                            std::uint32_t first_key,
                            std::uint32_t second_key) override {
        Check(table == 0x4000u + 32u && first_key == 0x11u &&
                  second_key == 0x122u, "lower bound arguments");
        calls.emplace_back("lower");
        return lower;
    }
    std::int32_t UpperBound(std::uint32_t table,
                            std::uint32_t first_key,
                            std::uint32_t second_key) override {
        Check(table == 0x4000u + 32u && first_key == 0x11u &&
                  second_key == 0x122u, "upper bound arguments");
        calls.emplace_back("upper");
        return upper;
    }
    void SetValidatedProperty(std::uint32_t actor,
                              std::uint32_t property,
                              std::int32_t value) override {
        Check(actor == 0x2000u && property == 5u && value == upper,
              "validated upper bound arguments");
        calls.emplace_back("validate");
        ++validation_calls;
        words[actor + 224u] = static_cast<std::uint32_t>(value);
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t count) override {
        Check(destination == 0x2000u && count == 144u,
              "event-data payload length and destination");
        calls.emplace_back("copy");
        for (std::uint32_t offset = 0u; offset < count; offset += 4u)
            words[destination + offset] = words[source + offset];
    }
    void CleanupMatchRange(std::uint32_t collection) override {
        Check(collection == 0x7000u, "range cleanup handle");
        calls.emplace_back("cleanup");
        ++cleanup_calls;
    }
    int FreeMatches(std::uint32_t collection) override {
        Check(collection == 0x7000u, "free match buffer handle");
        calls.emplace_back("free");
        ++free_calls;
        return -9;
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call absent");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void Prepare(Runtime& runtime, std::uint32_t count) {
    runtime.words[0x4000u + 44u] = 0x5000u;
    runtime.words[0x5000u + 96u] = 0x6000u;
    runtime.words[0x2000u] = 0xabcdef01u;
    runtime.count_schedule = {count};
}

void Predicate() {
    Runtime runtime;
    runtime.words[0x6000u + 152u] = 0x99u;
    Check(!pal4::CombatEventDataProcessor::Matches(
              0x6000u, 0x11u, 0x122u, runtime) &&
              runtime.calls == std::vector<std::string>{"read:24728"},
          "second key mismatch short circuits first-key read");
    runtime.calls.clear();
    runtime.words[0x6000u + 152u] = 0x122u;
    runtime.words[0x6000u + 148u] = 0x11u;
    Check(pal4::CombatEventDataProcessor::Matches(
              0x6000u, 0x11u, 0x122u, runtime) &&
              runtime.calls ==
                  std::vector<std::string>{"read:24728", "read:24724"},
          "both entry keys match in native read order");
}

void EmptyAndDynamicScan() {
    Runtime empty;
    Prepare(empty, 2u);
    Check(pal4::CombatEventDataProcessor::Process(
              0x2000u, 0x11u, 0x122u, empty) == -9 &&
              empty.collection_key == 0x22u && empty.database_reads == 1 &&
              empty.count_reads == 3 && empty.base_reads == 2 &&
              empty.matches.empty() && empty.cleanup_calls == 0 &&
              empty.free_calls == 1 && empty.calls.back() == "free",
          "empty scan frees collection without bound lookup or copy");

    Runtime changing;
    Prepare(changing, 3u);
    changing.count_schedule = {3u, 1u};
    Check(pal4::CombatEventDataProcessor::Process(
              0x2000u, 0x11u, 0x122u, changing) == -9 &&
              changing.count_reads == 2 && changing.base_reads == 1,
          "scan rereads dynamic count after each entry");
}

void FirstMatchAndBounds() {
    Runtime runtime;
    Prepare(runtime, 3u);
    const auto first = 0x6000u;
    const auto last = 0x6000u + 2u * 160u;
    runtime.words[first + 152u] = 0x122u;
    runtime.words[first + 148u] = 0x11u;
    runtime.words[last + 152u] = 0x122u;
    runtime.words[last + 148u] = 0x11u;
    runtime.words[first] = 0xdeadbeefu;
    runtime.words[first + 4u] = 0x11111111u;
    runtime.words[last + 4u] = 0x22222222u;
    runtime.words[0x2000u + 224u] = 1u;

    Check(pal4::CombatEventDataProcessor::Process(
              0x2000u, 0x11u, 0x122u, runtime) == -9 &&
              runtime.matches ==
                  std::vector<std::uint32_t>{first, last} &&
              runtime.count_reads == 4 && runtime.base_reads == 5 &&
              runtime.database_reads == 3 &&
              runtime.words[0x2000u + 224u] == 5u &&
              runtime.words[0x2000u] == 0xabcdef01u &&
              runtime.words[0x2000u + 4u] == 0x11111111u &&
              runtime.cleanup_calls == 1 && runtime.free_calls == 1,
          "first match copied, lower bound applied and vtable restored");
    Check(Index(runtime, "lower") < Index(runtime, "upper") &&
              Index(runtime, "upper") < Index(runtime, "write:8416") &&
              Index(runtime, "write:8416") < Index(runtime, "copy") &&
              Index(runtime, "copy") < Index(runtime, "write:8192") &&
              Index(runtime, "write:8192") < Index(runtime, "cleanup") &&
              Index(runtime, "cleanup") < Index(runtime, "free"),
          "bound, copy, vtable restore and cleanup order");

    Runtime upper;
    Prepare(upper, 1u);
    upper.words[first + 152u] = 0x122u;
    upper.words[first + 148u] = 0x11u;
    upper.words[0x2000u + 224u] = 20u;
    Check(pal4::CombatEventDataProcessor::Process(
              0x2000u, 0x11u, 0x122u, upper) == -9 &&
              upper.validation_calls == 1 &&
              upper.words[0x2000u + 224u] == 10u,
          "upper bound uses validated property setter");
}

void ExceptionCleanup() {
    Runtime runtime;
    Prepare(runtime, 1u);
    runtime.words[0x6000u + 152u] = 0x122u;
    runtime.words[0x6000u + 148u] = 0x11u;
    runtime.throw_on_insert = true;
    try {
        (void)pal4::CombatEventDataProcessor::Process(
            0x2000u, 0x11u, 0x122u, runtime);
        Check(false, "insert exception expected");
    } catch (const std::runtime_error&) {
        Check(runtime.free_calls == 1 && runtime.cleanup_calls == 0,
              "exception frees temporary collection exactly once");
    }
}

}  // namespace

int main() {
    try {
        Predicate();
        EmptyAndDynamicScan();
        FirstMatchAndBounds();
        ExceptionCleanup();
        std::cout << "combat event data processor tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
