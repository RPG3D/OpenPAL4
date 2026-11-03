#include "pal4/stunt_action_task.h"

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

struct Runtime final : pal4::IStuntActionCreationRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool no_memory = false;
    bool fail_base = false;
    bool fail_embedded = false;
    bool fail_text = false;
    bool fail_final = false;
    int fail_effect = -1;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::StuntActionTask::kSize, "outer allocation size");
        calls.push_back("allocate");
        return no_memory ? 0u : kObject;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "outer free address");
        calls.push_back("free");
    }
    void ConstructBase(std::uint32_t object) override {
        Check(object == kObject || object == kObject + 256u,
              "outer or embedded base address");
        const bool embedded = object != kObject;
        calls.push_back(embedded ? "embedded" : "base");
        if ((embedded && fail_embedded) || (!embedded && fail_base))
            throw std::runtime_error("base failure");
    }
    void DestroyCombatBase(std::uint32_t object) override {
        Check(object == kObject, "combat base address");
        calls.push_back("destroy_combat");
    }
    void DestroyGenericBase(std::uint32_t object) override {
        Check(object == kObject, "generic base address");
        calls.push_back("destroy_generic");
    }
    void DestroyMessageBase(std::uint32_t object) override {
        Check(object == kObject, "message base address");
        calls.push_back("destroy_message");
    }
    void DestroyEmbeddedBase(std::uint32_t object) override {
        Check(object == kObject + 256u, "embedded destructor address");
        calls.push_back("destroy_embedded");
    }
    void DestroyEffectSlot(std::uint32_t slot) override {
        calls.push_back("destroy_effect:" +
                        std::to_string((slot - kObject - 96u) / 24u));
    }
    void SetTextErrorFlag(std::uint32_t header, std::uint8_t flag) override {
        Check(header == kObject + 64u && flag == 0u, "text error flag");
        calls.push_back("text_flag");
        if (fail_text) throw std::runtime_error("text failure");
        for (std::uint32_t offset = 4u; offset < 16u; offset += 4u)
            words[header + offset] = 0u;
    }
    void ConstructNameNode(std::uint32_t slot) override {
        Check(slot >= kObject + 96u && slot <= kObject + 192u &&
                  (slot - kObject - 96u) % 24u == 0u,
              "effect slot stride");
        const auto index = (slot - kObject - 96u) / 24u;
        calls.push_back("effect:" + std::to_string(index));
        if (static_cast<int>(index) == fail_effect)
            throw std::runtime_error("effect failure");
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        if (fail_final && address == kObject &&
            value == pal4::StuntActionTask::kVtable)
            throw std::runtime_error("final write failure");
        words[address] = value;
        calls.push_back("word:" + std::to_string(value));
    }
};

std::size_t Index(const Runtime& r, const std::string& call) {
    const auto found = std::find(r.calls.begin(), r.calls.end(), call);
    Check(found != r.calls.end(), "missing call");
    return static_cast<std::size_t>(found - r.calls.begin());
}

void OuterLayout() {
    Runtime r;
    Check(pal4::StuntActionTask::Create(r) == kObject, "outer result");
    Check(r.words.at(kObject) == pal4::StuntActionTask::kVtable &&
              r.words.at(kObject + 256u) ==
                  pal4::StuntActionEmbeddedTask::kVtable,
          "outer and embedded vtables");
    Check(r.words.at(kObject + 60u) == 0u &&
              r.bytes.at(kObject + 64u) == 0u &&
              r.words.at(kObject + 68u) == 0u &&
              r.words.at(kObject + 72u) == 0u &&
              r.words.at(kObject + 76u) == 0u,
          "outer text fields");
    for (std::uint32_t offset = 80u; offset < 96u; offset += 4u)
        Check(r.words.at(kObject + offset) == 0u, "first records");
    for (std::uint32_t offset = 216u; offset < 256u; offset += 4u)
        Check(r.words.at(kObject + offset) == 0u, "trailing records");
    for (std::uint32_t i = 0u; i < 5u; ++i) {
        const auto slot = kObject + 96u + i * 24u;
        Check(r.bytes.at(slot + 8u) == 0u &&
                  r.words.at(slot + 12u) == 0u &&
                  r.words.at(slot + 16u) == 0u &&
                  r.words.at(slot + 20u) == 0u,
              "effect slot fields");
    }
    Check(Index(r, "base") < Index(r, "text_flag") &&
              Index(r, "text_flag") < Index(r, "effect:0") &&
              Index(r, "effect:4") < Index(r, "embedded"),
          "construction order");
}

void OuterFailures() {
    Runtime r;
    r.no_memory = true;
    Check(pal4::StuntActionTask::Create(r) == 0u &&
              r.calls == std::vector<std::string>{"allocate"},
          "allocation failure");
    r = Runtime{};
    r.fail_base = true;
    try { (void)pal4::StuntActionTask::Create(r); }
    catch (const std::runtime_error&) {}
    Check(r.calls == std::vector<std::string>({"allocate", "base", "free"}),
          "outer base failure");
    r = Runtime{};
    r.fail_text = true;
    try { (void)pal4::StuntActionTask::Create(r); }
    catch (const std::runtime_error&) {}
    Check(Index(r, "text_flag") < Index(r, "destroy_generic") &&
              r.calls.back() == "free", "text failure uses generic cleanup");
    for (int failed = 0; failed < 5; ++failed) {
        r = Runtime{};
        r.fail_effect = failed;
        try { (void)pal4::StuntActionTask::Create(r); }
        catch (const std::runtime_error&) {}
        for (int completed = failed - 1; completed >= 0; --completed)
            Check(Index(r, "destroy_effect:" + std::to_string(completed)) <
                      Index(r, "destroy_combat"),
                  "completed effect destroyed before base");
        if (failed >= 2)
            Check(Index(r, "destroy_effect:" + std::to_string(failed - 1)) <
                      Index(r, "destroy_effect:" + std::to_string(failed - 2)),
                  "effect destruction reversed");
        Check(r.calls.back() == "free", "effect failure frees object");
    }
    r = Runtime{};
    r.fail_embedded = true;
    try { (void)pal4::StuntActionTask::Create(r); }
    catch (const std::runtime_error&) {}
    Check(Index(r, "embedded") < Index(r, "destroy_message") &&
              r.calls.back() == "free", "embedded failure cleanup");
    r = Runtime{};
    r.fail_final = true;
    try { (void)pal4::StuntActionTask::Create(r); }
    catch (const std::runtime_error&) {}
    Check(Index(r, "destroy_embedded") < Index(r, "destroy_message") &&
              r.calls.back() == "free", "final write cleanup");
}

struct EmbeddedRuntime final : pal4::IStuntActionEmbeddedCreationRuntime {
    std::vector<std::string> calls;
    std::uint32_t allocated = kObject;
    bool fail_base = false;
    bool fail_write = false;
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::StuntActionEmbeddedTask::kSize, "76-byte allocation");
        calls.push_back("allocate");
        return allocated;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "embedded free identity");
        calls.push_back("free");
    }
    void ConstructBase(std::uint32_t object) override {
        Check(object == kObject, "embedded base identity");
        calls.push_back("base");
        if (fail_base) throw std::runtime_error("base failure");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == kObject &&
                  value == pal4::StuntActionEmbeddedTask::kVtable,
              "embedded vtable");
        calls.push_back("vtable");
        if (fail_write) throw std::runtime_error("write failure");
    }
};

void EmbeddedFactory() {
    EmbeddedRuntime r;
    Check(pal4::StuntActionEmbeddedTask::Create(r) == kObject &&
              r.calls == std::vector<std::string>({"allocate", "base", "vtable"}),
          "embedded factory success");
    r = EmbeddedRuntime{};
    r.allocated = 0u;
    Check(pal4::StuntActionEmbeddedTask::Create(r) == 0u &&
              r.calls == std::vector<std::string>{"allocate"},
          "embedded allocation failure");
    for (int stage = 0; stage < 2; ++stage) {
        r = EmbeddedRuntime{};
        r.fail_base = stage == 0;
        r.fail_write = stage == 1;
        try { (void)pal4::StuntActionEmbeddedTask::Create(r); }
        catch (const std::runtime_error&) {}
        Check(r.calls.back() == "free" && r.calls.size() ==
                  (stage == 0 ? 3u : 4u),
              "embedded failure frees allocation");
    }
}

}  // namespace

int main() {
    try {
        OuterLayout();
        OuterFailures();
        EmbeddedFactory();
        std::cout << "Stunt action creation tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
