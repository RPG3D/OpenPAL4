#include "pal4/magic_action_task.h"

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

struct Runtime final : pal4::IMagicActionCreationRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocation = kObject;
    bool fail_outer = false;
    bool fail_embedded = false;
    bool fail_final_write = false;
    int fail_effect = -1;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::MagicActionTask::kSize, "336-byte allocation");
        calls.push_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "free object identity");
        calls.push_back("free");
    }
    void ConstructBase(std::uint32_t object) override {
        Check(object == kObject || object == kObject + 256u,
              "outer or embedded base");
        const bool embedded = object != kObject;
        calls.push_back(embedded ? "embedded" : "base");
        if ((embedded && fail_embedded) || (!embedded && fail_outer))
            throw std::runtime_error("base failure");
    }
    void DestroyCombatBase(std::uint32_t object) override {
        Check(object == kObject, "combat base object");
        calls.push_back("destroy_combat_base");
    }
    void DestroyMessageBase(std::uint32_t object) override {
        Check(object == kObject, "message base object");
        calls.push_back("destroy_message_base");
    }
    void DestroyEmbeddedBase(std::uint32_t embedded) override {
        Check(embedded == kObject + 256u, "embedded base address");
        calls.push_back("destroy_embedded");
    }
    void DestroyEffectSlot(std::uint32_t slot) override {
        calls.push_back("destroy_effect:" +
                        std::to_string((slot - kObject - 96u) / 24u));
    }
    void SetTextErrorFlag(std::uint32_t header,
                          std::uint8_t flag) override {
        Check(header == kObject + 64u && flag == 0u,
              "text header and cleared flag");
        calls.push_back("set_text_flag");
        words[header + 4u] = 0u;
        words[header + 8u] = 0u;
        words[header + 12u] = 0u;
    }
    void ConstructNameNode(std::uint32_t slot) override {
        Check(slot >= kObject + 96u && slot <= kObject + 192u &&
                  (slot - kObject - 96u) % 24u == 0u,
              "five effect slots at 24-byte stride");
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
            value == pal4::MagicActionTask::kVtable)
            throw std::runtime_error("final write failure");
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

void LayoutAndComposition() {
    Runtime r;
    Check(pal4::MagicActionTask::Create(r) == kObject &&
              r.words[kObject] == pal4::MagicActionTask::kVtable &&
              r.words[kObject + 60u] == 0u &&
              r.bytes[kObject + 64u] == 0u &&
              r.words[kObject + 68u] == 0u &&
              r.words[kObject + 72u] == 0u &&
              r.words[kObject + 76u] == 0u &&
              r.words[kObject + 256u] == 0x845a64u,
          "outer, text and embedded vtable layout");
    for (std::uint32_t offset = 80u; offset < 96u; offset += 4u)
        Check(r.words[kObject + offset] == 0u,
              "two eight-byte records cleared");
    for (std::uint32_t offset = 216u; offset < 256u; offset += 4u)
        Check(r.words[kObject + offset] == 0u,
              "five embedded records cleared");
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto slot = kObject + 96u + index * 24u;
        Check(r.bytes[slot + 8u] == 0u &&
                  r.words[slot + 12u] == 0u &&
                  r.words[slot + 16u] == 0u &&
                  r.words[slot + 20u] == 0u,
              "existing effect-slot constructor used five times");
    }
    Check(Index(r, "base") < Index(r, "set_text_flag") &&
              Index(r, "set_text_flag") < Index(r, "effect:0") &&
              Index(r, "effect:4") < Index(r, "embedded") &&
              r.calls.back() == "write:4096" &&
              std::count(r.calls.begin(), r.calls.end(), "write:4096") == 3,
          "construction stage and vtable order");
}

void FailureCleanup() {
    Runtime r;
    r.allocation = 0u;
    Check(pal4::MagicActionTask::Create(r) == 0u &&
              r.calls == std::vector<std::string>{"allocate"},
          "allocation failure returns before construction");

    r = Runtime{};
    r.fail_outer = true;
    bool caught = false;
    try { (void)pal4::MagicActionTask::Create(r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && r.calls ==
              std::vector<std::string>{"allocate", "base", "free"},
          "failed outer base frees raw allocation");

    r = Runtime{};
    r.fail_effect = 2;
    caught = false;
    try { (void)pal4::MagicActionTask::Create(r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && Index(r, "effect:2") <
                        Index(r, "destroy_effect:1") &&
              Index(r, "destroy_effect:1") <
                  Index(r, "destroy_effect:0") &&
              Index(r, "destroy_effect:0") <
                  Index(r, "destroy_combat_base") &&
              r.calls.back() == "free" && !Called(r, "embedded"),
          "effect failure destroys completed slots in reverse order");

    r = Runtime{};
    r.fail_embedded = true;
    caught = false;
    try { (void)pal4::MagicActionTask::Create(r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && Index(r, "effect:4") < Index(r, "embedded") &&
              Index(r, "embedded") < Index(r, "destroy_message_base") &&
              !Called(r, "destroy_embedded") && r.calls.back() == "free",
          "embedded constructor failure destroys message base");

    r = Runtime{};
    r.fail_final_write = true;
    caught = false;
    try { (void)pal4::MagicActionTask::Create(r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && Index(r, "destroy_embedded") <
                        Index(r, "destroy_message_base") &&
              r.calls.back() == "free",
          "completed embedded base releases before message base");
}

}  // namespace

int main() {
    try {
        LayoutAndComposition();
        FailureCleanup();
        std::cout << "MagicAction factory tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
