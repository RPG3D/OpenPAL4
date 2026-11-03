#include "pal4/combat_event_collection.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEventCollectionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, char> text;
    std::vector<std::string> calls;
    std::uint32_t allocation = 8000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate " + std::to_string(bytes));
        return allocation;
    }
    void CopyConstruct(std::uint32_t destination,
                       std::uint32_t source) override {
        text[destination] = text[source];
        words[destination + 16u] = words[source + 16u];
        calls.push_back("copy " + std::to_string(destination) + " " +
                        std::to_string(source));
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        text[destination] = text[source];
        calls.push_back("assign " + std::to_string(destination) + " " +
                        std::to_string(source));
    }
    void ReleaseText(std::uint32_t record) override {
        calls.push_back("release " + std::to_string(record));
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
};

void Prepare(Runtime& runtime, std::uint32_t end,
             std::uint32_t capacity_end) {
    runtime.words[104u] = 1000u;
    runtime.words[108u] = end;
    runtime.words[112u] = capacity_end;
    runtime.text[5000u] = 'X';
    runtime.words[5016u] = 99u;
    for (std::uint32_t at = 1000u, id = 1u; at < end;
         at += 20u, ++id) {
        runtime.text[at] = static_cast<char>('A' + id - 1u);
        runtime.words[at + 16u] = id;
    }
}

void SpareCapacityWithShortTail() {
    Runtime runtime;
    Prepare(runtime, 1040u, 1100u);
    Check(pal4::CombatEventCollection::InsertCopies(
              100u, 1020u, 3u, 5000u, runtime) == 100u,
          "short tail returns collection address");
    Check(runtime.words[108u] == 1100u && runtime.words[112u] == 1100u,
          "short tail updates end only");
    Check(runtime.text[1000u] == 'A' && runtime.text[1020u] == 'X' &&
              runtime.text[1040u] == 'X' && runtime.text[1060u] == 'X' &&
              runtime.text[1080u] == 'B' && runtime.words[1096u] == 2u &&
              runtime.words[1036u] == 99u,
          "short tail keeps moved record and inserted metadata");
    Check(runtime.calls == std::vector<std::string>{
              "copy 1080 1020", "copy 1040 5000", "copy 1060 5000",
              "assign 1020 5000", "write 1036", "write 108"},
          "short tail copy and assignment order");
}

void SpareCapacityWithLongTail() {
    Runtime runtime;
    Prepare(runtime, 1080u, 1160u);
    Check(pal4::CombatEventCollection::InsertCopies(
              100u, 1020u, 2u, 5000u, runtime) == 40u,
          "long tail returns inserted byte count");
    Check(runtime.words[108u] == 1120u &&
              runtime.text[1000u] == 'A' &&
              runtime.text[1020u] == 'X' &&
              runtime.text[1040u] == 'X' &&
              runtime.text[1060u] == 'B' &&
              runtime.text[1080u] == 'C' &&
              runtime.text[1100u] == 'D' &&
              runtime.words[1076u] == 2u,
          "long tail moves backward and preserves metadata");
    Check(runtime.calls == std::vector<std::string>{
              "copy 1080 1040", "copy 1100 1060",
              "assign 1060 1020", "write 1076",
              "assign 1020 5000", "write 1036",
              "assign 1040 5000", "write 1056", "write 108"},
          "long tail copy and backward assignment order");
}

void ZeroCopies() {
    Runtime runtime;
    Prepare(runtime, 1040u, 1100u);
    Check(pal4::CombatEventCollection::InsertCopies(
              100u, 1000u, 0u, 5000u, runtime) == 0u &&
              runtime.calls.empty() && runtime.words[108u] == 1040u,
          "zero insertion has no effects");
}

void ReallocateExisting() {
    Runtime runtime;
    Prepare(runtime, 1060u, 1060u);
    Check(pal4::CombatEventCollection::InsertCopies(
              100u, 1020u, 2u, 5000u, runtime) == 0u,
          "reallocation return register is zero");
    Check(runtime.words[104u] == 8000u && runtime.words[108u] == 8100u &&
              runtime.words[112u] == 8120u &&
              runtime.text[8000u] == 'A' && runtime.text[8020u] == 'X' &&
              runtime.text[8040u] == 'X' && runtime.text[8060u] == 'B' &&
              runtime.text[8080u] == 'C',
          "reallocation copies prefix, insertion and suffix");
    Check(runtime.calls == std::vector<std::string>{
              "allocate 120", "copy 8000 1000", "copy 8020 5000",
              "copy 8040 5000", "copy 8060 1020", "copy 8080 1040",
              "release 1000", "release 1020", "release 1040",
              "free 1000", "write 112", "write 104", "write 108"},
          "reallocation cleanup and header write order");
}

void AllocateEmpty() {
    Runtime runtime;
    runtime.words[104u] = 0u;
    runtime.words[108u] = 0u;
    runtime.words[112u] = 0u;
    runtime.text[5000u] = 'X';
    runtime.words[5016u] = 99u;
    Check(pal4::CombatEventCollection::InsertCopies(
              100u, 0u, 2u, 5000u, runtime) == 0u &&
              runtime.words[104u] == 8000u &&
              runtime.words[108u] == 8040u &&
              runtime.words[112u] == 8040u &&
              runtime.text[8000u] == 'X' && runtime.text[8020u] == 'X',
          "empty collection allocates exact insertion capacity");
    Check(runtime.calls == std::vector<std::string>{
              "allocate 40", "copy 8000 5000", "copy 8020 5000",
              "free 0", "write 112", "write 104", "write 108"},
          "empty collection preserves free-null call and write order");
}

}  // namespace

int main() {
    try {
        SpareCapacityWithShortTail();
        SpareCapacityWithLongTail();
        ZeroCopies();
        ReallocateExisting();
        AllocateEmpty();
        std::cout << "combat event collection tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
