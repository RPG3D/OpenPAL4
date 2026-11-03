#include "pal4/effect_system_registration.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemRegistrationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x2010u, 0x4000u}, {0x2020u, 0x5000u},
        {0x1234u + 72u, 0xabcdu}};
    std::uint8_t active = 1u;
    std::uint32_t clump_candidate = 0x4000u;
    std::uint32_t atomic_candidate = 0x5000u;
    std::uint32_t callback = 0x63cea0u;
    bool insert_result = true;
    std::uint32_t erase_result = 0x12345678u;
    std::uint32_t traversal_result = 0x11223344u;
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x2004u, "active-byte offset");
        return active;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write");
        words[address] = value;
    }
    std::uint32_t FindLowerBound(
        pal4::EffectRegistrationTree kind, std::uint32_t tree,
        std::uint32_t key) override {
        Check((kind == pal4::EffectRegistrationTree::Atomics &&
               tree == 0x201cu && key == 0x1234u) ||
                  (kind == pal4::EffectRegistrationTree::Clumps &&
                   tree == 0x200cu && key == 0x7000u),
              "lookup uses expected tree and key");
        calls.push_back("find");
        return kind == pal4::EffectRegistrationTree::Atomics
                   ? atomic_candidate : clump_candidate;
    }
    bool InsertUnique(pal4::EffectRegistrationTree kind,
                      std::uint32_t tree, std::uint32_t key,
                      std::uint32_t saved) override {
        Check((kind == pal4::EffectRegistrationTree::Atomics &&
               tree == 0x201cu && key == 0x1234u &&
               saved == words.at(0x1234u + 72u)) ||
                  (kind == pal4::EffectRegistrationTree::Clumps &&
                   tree == 0x200cu && key == 0x7000u && saved == 0u),
              "insertion record preserves prior callback");
        calls.push_back("insert");
        return insert_result;
    }
    std::uint32_t Erase(pal4::EffectRegistrationTree kind,
                        std::uint32_t tree,
                        std::uint32_t node) override {
        Check((kind == pal4::EffectRegistrationTree::Atomics &&
               tree == 0x201cu && node == 0x5100u) ||
                  (kind == pal4::EffectRegistrationTree::Clumps &&
                   tree == 0x200cu && node == 0x4100u),
              "erasure selects registered node");
        calls.push_back("erase");
        return erase_result;
    }
    std::uint32_t EffectCallbackAddress() override { return callback; }
    std::uint32_t DefaultCallbackAddress() override { return 0xf00du; }
    std::uint32_t ProcessAllAtomics(
        std::uint32_t clump, std::uint8_t enabled) override {
        Check(clump == 0x7000u && enabled <= 1u,
              "traversal gets clump and state");
        calls.push_back(enabled ? "enable" : "disable");
        return traversal_result;
    }
};

void TestAtomicRegistration() {
    Runtime runtime;
    runtime.active = 0u;
    Check(pal4::EffectSystemRegistration::AddEffectToTree(
              0x2000u, 0x1234u, runtime) == 0u &&
              pal4::EffectSystemRegistration::RemoveEffectFromTree(
                  0x2000u, 0x1234u, runtime) == 0u &&
              runtime.calls.empty(),
          "disabled atomic operations do not touch tree");
    runtime.active = 1u;
    runtime.atomic_candidate = 0x5100u;
    runtime.words[0x510cu] = 0x1234u;
    Check(pal4::EffectSystemRegistration::AddEffectToTree(
              0x2000u, 0x1234u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"find"},
          "duplicate atomic keeps existing callback");
    runtime.calls.clear();
    runtime.atomic_candidate = 0x5000u;
    runtime.insert_result = false;
    (void)pal4::EffectSystemRegistration::AddEffectToTree(
        0x2000u, 0x1234u, runtime);
    Check(runtime.words[0x1234u + 72u] == 0xabcdu &&
              runtime.calls == std::vector<std::string>{"find", "insert"},
          "failed insertion leaves callback unchanged");
    runtime.calls.clear();
    runtime.insert_result = true;
    (void)pal4::EffectSystemRegistration::AddEffectToTree(
        0x2000u, 0x1234u, runtime);
    Check(runtime.words[0x1234u + 72u] == runtime.callback &&
              runtime.calls ==
                  std::vector<std::string>{"find", "insert", "write"},
          "new node installs effect callback after insertion");
    runtime.atomic_candidate = 0x5100u;
    runtime.words[0x510cu] = 0x1234u;
    runtime.words[0x5110u] = 0xabcdu;
    runtime.calls.clear();
    Check(pal4::EffectSystemRegistration::RemoveEffectFromTree(
              0x2000u, 0x1234u, runtime) == 0x78u &&
              runtime.words[0x1234u + 72u] == 0xabcdu &&
              runtime.calls ==
                  std::vector<std::string>{"find", "write", "erase"},
          "removal restores callback before erasing node");
    runtime.words[0x5110u] = 0u;
    runtime.calls.clear();
    (void)pal4::EffectSystemRegistration::RemoveEffectFromTree(
        0x2000u, 0x1234u, runtime);
    Check(runtime.words[0x1234u + 72u] == 0xf00du &&
              runtime.calls ==
                  std::vector<std::string>{"find", "write", "write",
                                           "erase"},
          "null saved callback selects default");
    runtime.atomic_candidate = 0x5000u;
    runtime.calls.clear();
    Check(pal4::EffectSystemRegistration::RemoveEffectFromTree(
              0x2000u, 0x1234u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"find"},
          "missing node returns head low byte");
}

void TestClumpRegistration() {
    Runtime runtime;
    runtime.active = 0u;
    Check(!pal4::EffectSystemRegistration::HandleEffectAddition(
              0x2000u, 0x7000u, runtime) &&
              pal4::EffectSystemRegistration::RemoveClumpAndProcessAtomics(
                  0x2000u, 0x7000u, runtime) == 0u,
          "disabled clump registration is inert");
    runtime.active = 1u;
    runtime.clump_candidate = 0x4100u;
    runtime.words[0x410cu] = 0x7000u;
    Check(pal4::EffectSystemRegistration::HandleEffectAddition(
              0x2000u, 0x7000u, runtime) &&
              runtime.calls == std::vector<std::string>{"find"},
          "duplicate clump skips atomic traversal");
    runtime.calls.clear();
    runtime.clump_candidate = 0x4000u;
    runtime.insert_result = false;
    Check(pal4::EffectSystemRegistration::HandleEffectAddition(
              0x2000u, 0x7000u, runtime) &&
              runtime.calls ==
                  std::vector<std::string>{"find", "insert", "enable"},
          "second-stage duplicate still enables atomics");
    runtime.calls.clear();
    runtime.insert_result = true;
    Check(pal4::EffectSystemRegistration::HandleEffectAddition(
              0x2000u, 0x7000u, runtime) &&
              runtime.calls ==
                  std::vector<std::string>{"find", "insert", "enable"},
          "new clump enables atomics after insertion");
    runtime.calls.clear();
    runtime.clump_candidate = 0x4100u;
    Check(pal4::EffectSystemRegistration::RemoveClumpAndProcessAtomics(
              0x2000u, 0x7000u, runtime) == 0x44u &&
              runtime.calls ==
                  std::vector<std::string>{"find", "erase", "disable"},
          "removal erases before disabling atomics and returns low byte");
    runtime.clump_candidate = 0x4000u;
    runtime.calls.clear();
    Check(pal4::EffectSystemRegistration::RemoveClumpAndProcessAtomics(
              0x2000u, 0x7000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"find"},
          "missing clump returns head low byte");
}

}  // namespace

int main() {
    TestAtomicRegistration();
    TestClumpRegistration();
}
