#include "pal4/win_action_child_task.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kObject = 0x1000u;

struct Runtime final : pal4::IWinActionChildCreationRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocation = kObject;
    std::uint32_t requested_size = 0u;
    bool fail_base = false;
    bool fail_final_write = false;
    int fail_effect = -1;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 64u || size == 216u, "WinAction child allocation size");
        requested_size = size;
        calls.push_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "child allocation released");
        calls.push_back("free");
    }
    void ConstructBase(std::uint32_t object) override {
        Check(object == kObject, "generic base object address");
        calls.push_back("base");
        if (fail_base) throw std::runtime_error("base failure");
    }
    void DestroyBase(std::uint32_t object) override {
        Check(object == kObject, "generic base cleanup address");
        calls.push_back("destroy_base");
    }
    void DestroyEffectSlot(std::uint32_t slot) override {
        calls.push_back("destroy_effect:" +
                        std::to_string((slot - kObject - 96u) / 24u));
    }
    void DestroyMessageBase(std::uint32_t object) override {
        Check(object == kObject, "message base cleanup address");
        calls.push_back("destroy_message_base");
    }
    void ConstructNameNode(std::uint32_t slot) override {
        Check(slot >= kObject + 96u && slot <= kObject + 192u &&
                  (slot - kObject - 96u) % 24u == 0u,
              "five 24-byte effect slots");
        const auto index = (slot - kObject - 96u) / 24u;
        calls.push_back("effect:" + std::to_string(index));
        if (static_cast<int>(index) == fail_effect)
            throw std::runtime_error("effect failure");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("write_byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        if (fail_final_write && address == kObject &&
            (value == pal4::WinActionSecondChildTask::kVtable ||
             value == pal4::WinActionThirdChildTask::kVtable))
            throw std::runtime_error("vtable write failure");
        words[address] = value;
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

bool Called(const Runtime& runtime, const std::string& call) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), call) !=
           runtime.calls.end();
}

void SecondChild() {
    Runtime r;
    Check(pal4::WinActionSecondChildTask::Create(r) == kObject &&
              r.requested_size == 64u && r.words[kObject + 60u] == 0u &&
              r.words[kObject] == 0x845a88u &&
              r.calls == std::vector<std::string>{
                  "allocate", "base", "write:4156", "write:4096"},
          "second child base, actor field and vtable");

    r = Runtime{};
    r.allocation = 0u;
    Check(pal4::WinActionSecondChildTask::Create(r) == 0u &&
              r.calls == std::vector<std::string>{"allocate"},
          "second child allocation failure");

    r = Runtime{};
    r.fail_base = true;
    bool caught = false;
    try { (void)pal4::WinActionSecondChildTask::Create(r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && r.calls ==
              std::vector<std::string>{"allocate", "base", "free"},
          "failed second-child base frees raw storage");

    r = Runtime{};
    r.fail_final_write = true;
    caught = false;
    try { (void)pal4::WinActionSecondChildTask::Create(r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && Index(r, "destroy_base") < Index(r, "free"),
          "second-child vtable failure cleans base before storage");
}

void ThirdChild() {
    Runtime r;
    Check(pal4::WinActionThirdChildTask::Create(r) == kObject &&
              r.requested_size == 216u &&
              r.words[kObject] == 0x8456c4u &&
              r.words[kObject + 60u] == 0u &&
              r.bytes[kObject + 64u] == 0u &&
              r.words[kObject + 68u] == 0u &&
              r.words[kObject + 72u] == 0u &&
              r.words[kObject + 76u] == 0u,
          "third child size, text fields and message vtable");
    for (std::uint32_t offset = 80u; offset < 96u; offset += 4u)
        Check(r.words[kObject + offset] == 0u,
              "two eight-byte records cleared");
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto slot = kObject + 96u + index * 24u;
        Check(r.bytes[slot + 8u] == 0u &&
                  r.words[slot + 12u] == 0u &&
                  r.words[slot + 16u] == 0u &&
                  r.words[slot + 20u] == 0u,
              "five composed effect slots cleared");
    }
    Check(Index(r, "base") < Index(r, "effect:0") &&
              r.calls.back() == "write:4096" &&
              std::count(r.calls.begin(), r.calls.end(), "write:4096") == 2,
          "construction and final vtable stages");

    r = Runtime{};
    r.allocation = 0u;
    Check(pal4::WinActionThirdChildTask::Create(r) == 0u &&
              r.calls == std::vector<std::string>{"allocate"},
          "third child allocation failure");

    r = Runtime{};
    r.fail_base = true;
    bool caught = false;
    try { (void)pal4::WinActionThirdChildTask::Create(r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && r.calls ==
              std::vector<std::string>{"allocate", "base", "free"},
          "failed third-child base frees raw storage");

    r = Runtime{};
    r.fail_effect = 2;
    caught = false;
    try { (void)pal4::WinActionThirdChildTask::Create(r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && Index(r, "effect:2") <
                        Index(r, "destroy_effect:1") &&
              Index(r, "destroy_effect:1") <
                  Index(r, "destroy_effect:0") &&
              Index(r, "destroy_effect:0") < Index(r, "destroy_base") &&
              r.calls.back() == "free",
          "effect failure unwinds prior slots in reverse order");

    r = Runtime{};
    r.fail_final_write = true;
    caught = false;
    try { (void)pal4::WinActionThirdChildTask::Create(r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && Index(r, "effect:4") <
                        Index(r, "destroy_base") &&
              r.calls.back() == "free" &&
              !Called(r, "destroy_message_base"),
          "failure before final vtable unwinds constructed effect slots");
}

}  // namespace

int main() {
    try {
        SecondChild();
        ThirdChild();
        std::cout << "WinAction child task tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
