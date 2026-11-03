#include "pal4/combat_result_child_tasks.h"

#include <algorithm>
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

constexpr std::uint32_t kObject = 0x1000u;

struct Runtime final : pal4::ICombatResultChildRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocation = kObject;
    std::uint32_t requested_size = 0u;
    bool fail_base = false;
    int fail_write = -1;
    int write_count = 0;

    std::uint32_t Allocate(std::uint32_t size) override {
        calls.emplace_back("allocate");
        requested_size = size;
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "free result child storage");
        calls.emplace_back("free");
    }
    void ConstructBase(std::uint32_t object) override {
        Check(object == kObject, "result child base address");
        calls.emplace_back("construct_base");
        if (fail_base) throw std::runtime_error("base constructor");
    }
    void DestroyBase(std::uint32_t object) override {
        Check(object == kObject, "result child base destructor address");
        calls.emplace_back("destroy_base");
    }
    void MaybeFailWrite() {
        if (write_count++ == fail_write)
            throw std::runtime_error("field write");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("word:" + std::to_string(address));
        MaybeFailWrite();
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address));
        MaybeFailWrite();
        bytes[address] = value;
    }
};

std::uint32_t Create(bool message, Runtime& runtime) {
    return message
        ? pal4::MissMessageChildTask::Create(runtime)
        : pal4::FendOffImpactChildTask::Create(runtime);
}

void Layout(bool message) {
    Runtime runtime;
    Check(Create(message, runtime) == kObject &&
              runtime.requested_size == (message ? 80u : 68u) &&
              runtime.words.at(kObject) == (message
                  ? pal4::MissMessageChildTask::kVtable
                  : pal4::FendOffImpactChildTask::kVtable),
          "result child size and vtable");
    if (message) {
        Check(runtime.words.at(kObject + 60u) == 0u &&
                  runtime.bytes.at(kObject + 64u) == 0u &&
                  runtime.words.at(kObject + 68u) == 0u &&
                  runtime.words.at(kObject + 72u) == 0u &&
                  runtime.words.at(kObject + 76u) == 0u &&
                  runtime.calls == std::vector<std::string>{
                      "allocate", "construct_base", "word:4156",
                      "byte:4160", "word:4164", "word:4168",
                      "word:4172", "word:4096"},
              "80-byte message child zero fields and IDA write order");
    } else {
        Check(runtime.calls == std::vector<std::string>{
                  "allocate", "construct_base", "word:4096"},
              "68-byte impact child base and vtable order");
    }

    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(Create(message, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "null allocation skips base construction");
}

void FailureCleanup(bool message) {
    Runtime runtime;
    runtime.fail_base = true;
    bool caught = false;
    try { (void)Create(message, runtime); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate", "construct_base", "free"},
          "base failure frees raw child storage");

    for (int write = 0; write < (message ? 6 : 1); ++write) {
        runtime = Runtime{};
        runtime.fail_write = write;
        caught = false;
        try { (void)Create(message, runtime); }
        catch (const std::runtime_error&) { caught = true; }
        Check(caught && runtime.calls.size() >= 5u &&
                  runtime.calls[runtime.calls.size() - 2u] ==
                      "destroy_base" &&
                  runtime.calls.back() == "free",
              "field failure destroys constructed base before free");
    }
}

std::uint32_t CreateFlee(bool candidate, Runtime& runtime) {
    return candidate ? pal4::FleeCandidateChildTask::Create(runtime)
                     : pal4::FleeFinishChildTask::Create(runtime);
}

void FleeChildConstruction(bool candidate) {
    Runtime runtime;
    Check(CreateFlee(candidate, runtime) == kObject &&
              runtime.requested_size == (candidate ? 84u : 68u) &&
              runtime.words.at(kObject) == (candidate
                  ? pal4::FleeCandidateChildTask::kVtable
                  : pal4::FleeFinishChildTask::kVtable),
          "Flee child size and vtable");
    if (candidate) {
        Check(runtime.words.at(kObject + 60u) == 0u &&
                  runtime.bytes.at(kObject + 64u) == 0u &&
                  runtime.words.at(kObject + 68u) == 0u &&
                  runtime.words.at(kObject + 72u) == 0u &&
                  runtime.words.at(kObject + 76u) == 0u &&
                  runtime.bytes.at(kObject + 80u) == 0u &&
                  runtime.calls == std::vector<std::string>{
                      "allocate", "construct_base", "word:4156",
                      "byte:4160", "word:4164", "word:4168",
                      "word:4172", "byte:4176", "word:4096"},
              "candidate child initializes six fields before vtable");
    } else {
        Check(runtime.calls == std::vector<std::string>{
                  "allocate", "construct_base", "word:4096"},
              "finish child only installs vtable after base");
    }
    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(CreateFlee(candidate, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "Flee null allocation skips base");
    runtime = Runtime{};
    runtime.fail_base = true;
    bool caught = false;
    try { (void)CreateFlee(candidate, runtime); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate", "construct_base", "free"},
          "Flee failed base frees allocation");
    for (int write = 0; write < (candidate ? 7 : 1); ++write) {
        runtime = Runtime{};
        runtime.fail_write = write;
        caught = false;
        try { (void)CreateFlee(candidate, runtime); }
        catch (const std::runtime_error&) { caught = true; }
        Check(caught && runtime.calls[runtime.calls.size() - 2u] ==
                            "destroy_base" && runtime.calls.back() == "free",
              "Flee field failure destroys base before free");
    }
}

struct FleeDestructionRuntime final
    : pal4::IFleeCandidateChildDestructionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read_word");
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read_byte");
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.emplace_back("decrement");
        bytes[address] = value;
    }
    void FreeText(std::uint32_t header) override {
        Check(header == 0x4fffu, "Flee text header address");
        calls.emplace_back("free_text");
    }
    std::uint32_t DestroyGenericBase(std::uint32_t object) override {
        Check(object == kObject, "Flee generic base address");
        calls.emplace_back("destroy_generic");
        return 0x7777u;
    }
};

void FleeCandidateDestruction() {
    for (int count : {-1, 0, 1, 3, 255}) {
        FleeDestructionRuntime runtime;
        runtime.words[kObject + 68u] = count < 0 ? 0u : 0x5000u;
        if (count >= 0)
            runtime.bytes[0x4fffu] = static_cast<std::uint8_t>(count);
        Check(pal4::FleeCandidateChildTask::DestroyBase(
                  kObject, runtime) == 0x7777u &&
                  runtime.words.at(kObject + 68u) == 0u &&
                  runtime.words.at(kObject + 72u) == 0u &&
                  runtime.words.at(kObject + 76u) == 0u &&
                  runtime.calls.back() == "destroy_generic",
              "Flee candidate destruction clears text and forwards base result");
        const bool free_text = count == 0 || count == 255;
        const bool decrement = count == 1 || count == 3;
        Check((std::find(runtime.calls.begin(), runtime.calls.end(),
                         "free_text") != runtime.calls.end()) == free_text &&
                  (std::find(runtime.calls.begin(), runtime.calls.end(),
                             "decrement") != runtime.calls.end()) == decrement,
              "Flee text reference count branch");
        if (decrement)
            Check(runtime.bytes.at(0x4fffu) == count - 1,
                  "Flee text reference decrements exactly once");
    }
}

}  // namespace

int main() {
    for (bool message : {false, true}) {
        Layout(message);
        FailureCleanup(message);
        FleeChildConstruction(message);
    }
    FleeCandidateDestruction();
    std::cout << "Combat result child task tests passed\n";
}
