#include "pal4/combat_fatal_child_creation.h"

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

struct Runtime final : pal4::IFatalChildCreationRuntime {
    std::uint32_t allocated_size = 0u;
    std::uint32_t allocation = kObject;
    std::uint32_t fail_write = 0xffffffffu;
    bool fail_base = false;
    bool fail_name = false;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t size) override {
        allocated_size = size;
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "free address");
        calls.emplace_back("free");
    }
    void ConstructBase(std::uint32_t object) override {
        Check(object == kObject, "base address");
        calls.emplace_back("base");
        if (fail_base) throw std::runtime_error("base failure");
    }
    void DestroyBase(std::uint32_t object) override {
        Check(object == kObject, "destroy base address");
        calls.emplace_back("destroy_base");
    }
    void ConstructNameNode(std::uint32_t node) override {
        Check(node == kObject + 80u, "name node address");
        calls.emplace_back("name");
        if (fail_name) throw std::runtime_error("name failure");
    }
    void ResetNameNode(std::uint32_t node) override {
        Check(node == kObject + 80u, "reset name address");
        calls.emplace_back("reset_name");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("word:" + std::to_string(address));
        if (address == fail_write)
            throw std::runtime_error("write failure");
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
};

using Create = std::uint32_t (*)(pal4::IFatalChildCreationRuntime&);

struct Case {
    Create create;
    std::uint32_t size;
    std::uint32_t vtable;
    std::vector<std::uint32_t> zero_words;
    bool has_name;
    bool vtable_last;
};

bool Called(const Runtime& runtime, const std::string& call) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), call) !=
           runtime.calls.end();
}

void VerifyCase(const Case& item) {
    Runtime r;
    Check(item.create(r) == kObject &&
              r.allocated_size == item.size &&
              r.words.at(kObject) == item.vtable &&
              r.calls[0] == "allocate" &&
              r.calls[1] == "base" &&
              (item.vtable_last
                  ? r.calls.back() == "word:4096"
                  : r.calls[2] == "word:4096"),
          "child size, vtable and base order");
    for (const auto offset : item.zero_words)
        Check(r.words.at(kObject + offset) == 0u,
              "zero initialized child word");
    Check(Called(r, "name") == item.has_name,
          "name node stage");
    if (item.has_name) {
        Check(r.bytes.at(kObject + 60u) == 0u &&
                  r.bytes.at(kObject + 88u) == 0u,
              "defined message child bytes");
    }

    r = Runtime{};
    r.allocation = 0u;
    Check(item.create(r) == 0u &&
              r.calls == std::vector<std::string>{"allocate"},
          "null allocation");

    r = Runtime{};
    r.fail_base = true;
    bool caught = false;
    try { (void)item.create(r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && r.calls.back() == "free" &&
              !Called(r, "destroy_base"),
          "base failure frees raw memory");

    r = Runtime{};
    r.fail_write = kObject;
    caught = false;
    try { (void)item.create(r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && r.calls.back() == "free" &&
              Called(r, "destroy_base") &&
              Called(r, "reset_name") == item.has_name,
          "vtable failure reverse cleanup");
    if (item.has_name) {
        const auto reset = std::find(r.calls.begin(), r.calls.end(),
                                     "reset_name");
        const auto base = std::find(r.calls.begin(), r.calls.end(),
                                    "destroy_base");
        Check(reset < base, "name before base cleanup");
    }
}

void MessageNameFailure() {
    Runtime r;
    r.fail_name = true;
    bool caught = false;
    try { (void)pal4::FatalMessageChildTask::Create(r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && r.calls.back() == "free" &&
              Called(r, "destroy_base") &&
              !Called(r, "reset_name"),
          "name construction failure cleans base and storage");
    Check(r.words.at(kObject + 64u) == 0u &&
              r.words.at(kObject + 68u) == 0u &&
              r.words.at(kObject + 72u) == 0u &&
              r.words.at(kObject + 76u) == 0u,
          "message prefix before name");
}

}  // namespace

int main() {
    try {
        for (const Case& item : std::vector<Case>{
                 {pal4::FatalEventChildTask::Create, 72u, 0x845df4u,
                  {64u, 60u}, false, false},
                 {pal4::FatalFirstFinalChildTask::Create, 64u,
                  0x845e10u, {60u}, false, true},
                 {pal4::FatalFinishChildTask::Create, 80u,
                  0x845e2cu, {60u}, false, true},
                 {pal4::FatalEndChildTask::Create, 68u,
                  0x845e48u, {}, false, true},
                 {pal4::FatalMessageChildTask::Create, 100u,
                  0x845788u, {64u, 68u, 72u, 76u, 92u, 96u},
                  true, true}})
            VerifyCase(item);
        MessageNameFailure();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
