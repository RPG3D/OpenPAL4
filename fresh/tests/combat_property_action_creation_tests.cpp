#include "pal4/combat_property_action_creation.h"
#include "pal4/cast_property_action_task.h"
#include "pal4/property_action_task.h"
#include "pal4/throw_property_action_task.h"
#include "pal4/throw_property_task.h"
#include "pal4/use_property_task.h"

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

struct Runtime final : pal4::ICombatPropertyActionCreationRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocation = kObject;
    std::uint32_t expected_size = 0u;
    std::uint32_t fail_base_at = 0u;
    std::uint32_t fail_name_at = 0u;
    std::uint32_t fail_write_at = 0u;
    std::uint32_t fail_write_value = 0u;
    bool fail_text = false;

    void FailIf(bool condition) {
        if (condition) throw std::runtime_error("injected construction failure");
    }
    std::uint32_t Allocate(std::uint32_t bytes_count) override {
        Check(bytes_count == expected_size, "allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "free outer object");
        calls.emplace_back("free");
    }
    void ConstructBase(std::uint32_t object) override {
        calls.push_back("base:" + std::to_string(object));
        FailIf(object == fail_base_at);
    }
    void DestroyGenericBase(std::uint32_t object) override {
        calls.push_back("destroy_generic:" + std::to_string(object));
    }
    void DestroyCombatBase(std::uint32_t object) override {
        Check(object == kObject, "combat base identity");
        calls.emplace_back("destroy_combat");
    }
    void DestroyMessageBase(std::uint32_t object) override {
        Check(object == kObject, "message base identity");
        calls.emplace_back("destroy_message");
    }
    void DestroyEffectSlot(std::uint32_t slot) override {
        calls.push_back("destroy_effect:" +
                        std::to_string((slot - kObject - 96u) / 24u));
    }
    void SetTextErrorFlag(std::uint32_t header,
                          std::uint8_t flag) override {
        Check(header == kObject + 64u && flag == 0u, "text flag");
        calls.emplace_back("text");
        FailIf(fail_text);
        for (auto offset : {4u, 8u, 12u}) words[header + offset] = 0u;
    }
    void ConstructNameNode(std::uint32_t node) override {
        calls.push_back("name:" + std::to_string(node));
        FailIf(node == fail_name_at);
        words[node] = 0xffffffffu;
        words[node + 4u] = 0u;
    }
    void ResetNameNode(std::uint32_t node) override {
        calls.push_back("reset_name:" + std::to_string(node));
    }
    void CleanupCastEmbedded(std::uint32_t embedded) override {
        Check(embedded == kObject + 232u, "cast embedded address");
        calls.emplace_back("cleanup_cast");
    }
    void DestroyThrowProperty(std::uint32_t embedded) override {
        Check(embedded == kObject + 240u, "throw destructor address");
        calls.emplace_back("destroy_throw");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        FailIf(address == fail_write_at && value == fail_write_value);
        words[address] = value;
    }
};

std::size_t Index(const Runtime& r, const std::string& call) {
    const auto found = std::find(r.calls.begin(), r.calls.end(), call);
    Check(found != r.calls.end(), "missing cleanup call");
    return static_cast<std::size_t>(found - r.calls.begin());
}

void CommonLayout(const Runtime& r, std::uint32_t outer_vtable) {
    Check(r.words.at(kObject) == outer_vtable &&
              r.words.at(kObject + 60u) == 0u &&
              r.bytes.at(kObject + 64u) == 0u &&
              r.words.at(kObject + 68u) == 0u &&
              r.words.at(kObject + 72u) == 0u &&
              r.words.at(kObject + 76u) == 0u,
          "shared action base and text fields");
    for (auto offset : {80u, 84u, 88u, 92u, 216u, 220u, 224u, 228u})
        Check(r.words.at(kObject + offset) == 0u,
              "shared zeroed records");
    for (std::uint32_t i = 0u; i < 5u; ++i) {
        const auto slot = kObject + 96u + 24u * i;
        Check(r.words.at(slot) == 0xffffffffu &&
                  r.words.at(slot + 4u) == 0u &&
                  r.bytes.at(slot + 8u) == 0u &&
                  r.words.at(slot + 12u) == 0u &&
                  r.words.at(slot + 16u) == 0u &&
                  r.words.at(slot + 20u) == 0u,
              "five effect slots at 24-byte stride");
    }
    Check(Index(r, "text") < Index(r, "name:4192") &&
              Index(r, "name:4288") < r.calls.size() - 1u &&
              r.calls.back() == "write:4096",
          "shared prefix ordering");
}

void SuccessLayouts() {
    Runtime r;
    r.expected_size = pal4::CastPropertyActionTask::kSize;
    Check(pal4::CastPropertyActionTask::Create(r) == kObject,
          "cast allocation result");
    CommonLayout(r, pal4::CastPropertyActionTask::kVtable);
    Check(r.words.at(kObject + 232u) == pal4::UsePropertyTask::kVtable &&
              r.words.at(kObject + 304u) == 0xffffffffu &&
              r.words.at(kObject + 312u) == 0xffffffffu,
          "cast embedded base and two name nodes");

    r = Runtime{};
    r.expected_size = pal4::ThrowPropertyActionTask::kSize;
    Check(pal4::ThrowPropertyActionTask::Create(r) == kObject,
          "throw allocation result");
    CommonLayout(r, pal4::ThrowPropertyActionTask::kVtable);
    Check(r.words.at(kObject + 232u) == 0xffffffffu &&
              r.words.at(kObject + 240u) ==
                  pal4::ThrowPropertyTask::kVtable &&
              r.words.at(kObject + 328u) ==
                  pal4::UsePropertyTask::kVtable &&
              r.words.at(kObject + 400u) == 0xffffffffu,
          "throw embedded component, property and handles");

    r = Runtime{};
    r.expected_size = pal4::PropertyActionTask::kSize;
    Check(pal4::PropertyActionTask::Create(r) == kObject,
          "direct allocation result");
    CommonLayout(r, pal4::PropertyActionTask::kVtable);
    Check(r.words.at(kObject + 232u) == 0xffffffffu &&
              r.words.at(kObject + 240u) ==
                  pal4::UsePropertyTask::kVtable &&
              r.words.at(kObject + 312u) == 0xffffffffu,
          "direct embedded property and handle nodes");

    r = Runtime{};
    r.expected_size = pal4::UsePropertyTask::kSize;
    Check(pal4::UsePropertyTask::Create(r) == kObject &&
              r.words.at(kObject) == pal4::UsePropertyTask::kVtable &&
              r.words.at(kObject + 72u) == 0xffffffffu &&
              r.words.at(kObject + 76u) == 0u,
          "standalone 80-byte property task layout");
}

template <typename Create>
void ExpectFailure(Runtime& r, Create create,
                   const std::string& cleanup) {
    bool caught = false;
    try { (void)create(r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && Index(r, cleanup) < Index(r, "free") &&
              r.calls.back() == "free", "failed stage cleanup and free");
}

void CommonFailures() {
    Runtime r;
    r.expected_size = pal4::CastPropertyActionTask::kSize;
    r.allocation = 0u;
    Check(pal4::CastPropertyActionTask::Create(r) == 0u &&
              r.calls == std::vector<std::string>{"allocate"},
          "null outer allocation");
    r = Runtime{};
    r.expected_size = pal4::CastPropertyActionTask::kSize;
    r.fail_base_at = kObject;
    bool caught = false;
    try { (void)pal4::CastPropertyActionTask::Create(r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && r.calls == std::vector<std::string>({
              "allocate", "base:4096", "free"}),
          "outer base failure frees raw allocation");
    r = Runtime{};
    r.expected_size = pal4::CastPropertyActionTask::kSize;
    r.fail_text = true;
    ExpectFailure(r, pal4::CastPropertyActionTask::Create,
                  "destroy_generic:4096");
    r = Runtime{};
    r.expected_size = pal4::CastPropertyActionTask::kSize;
    r.fail_name_at = kObject + 96u + 2u * 24u;
    ExpectFailure(r, pal4::CastPropertyActionTask::Create,
                  "destroy_combat");
    Check(Index(r, "destroy_effect:1") < Index(r, "destroy_effect:0") &&
              Index(r, "destroy_effect:0") < Index(r, "destroy_combat"),
          "completed effect slots destroyed in reverse order");
}

void TailFailures() {
    Runtime r;
    r.expected_size = pal4::CastPropertyActionTask::kSize;
    r.fail_base_at = kObject + 232u;
    ExpectFailure(r, pal4::CastPropertyActionTask::Create,
                  "destroy_message");
    r = Runtime{};
    r.expected_size = pal4::CastPropertyActionTask::kSize;
    r.fail_name_at = kObject + 304u;
    ExpectFailure(r, pal4::CastPropertyActionTask::Create,
                  "destroy_generic:4328");
    r = Runtime{};
    r.expected_size = pal4::CastPropertyActionTask::kSize;
    r.fail_name_at = kObject + 312u;
    ExpectFailure(r, pal4::CastPropertyActionTask::Create,
                  "cleanup_cast");
    r = Runtime{};
    r.expected_size = pal4::CastPropertyActionTask::kSize;
    r.fail_write_at = kObject;
    r.fail_write_value = pal4::CastPropertyActionTask::kVtable;
    ExpectFailure(r, pal4::CastPropertyActionTask::Create,
                  "reset_name:4408");
    Check(Index(r, "reset_name:4408") < Index(r, "cleanup_cast") &&
              Index(r, "cleanup_cast") < Index(r, "destroy_message"),
          "cast final failure cleanup order");

    r = Runtime{};
    r.expected_size = pal4::ThrowPropertyActionTask::kSize;
    r.fail_name_at = kObject + 232u;
    ExpectFailure(r, pal4::ThrowPropertyActionTask::Create,
                  "destroy_message");
    r = Runtime{};
    r.expected_size = pal4::ThrowPropertyActionTask::kSize;
    r.fail_base_at = kObject + 240u;
    ExpectFailure(r, pal4::ThrowPropertyActionTask::Create,
                  "reset_name:4328");
    r = Runtime{};
    r.expected_size = pal4::ThrowPropertyActionTask::kSize;
    r.fail_base_at = kObject + 328u;
    ExpectFailure(r, pal4::ThrowPropertyActionTask::Create,
                  "destroy_throw");
    Check(Index(r, "destroy_throw") < Index(r, "reset_name:4328"),
          "throw component releases before first name");
    r = Runtime{};
    r.expected_size = pal4::ThrowPropertyActionTask::kSize;
    r.fail_name_at = kObject + 400u;
    ExpectFailure(r, pal4::ThrowPropertyActionTask::Create,
                  "destroy_generic:4424");
    Check(Index(r, "destroy_generic:4424") < Index(r, "destroy_throw"),
          "throw embedded base releases before throw component");
    r = Runtime{};
    r.expected_size = pal4::ThrowPropertyActionTask::kSize;
    r.fail_write_at = kObject;
    r.fail_write_value = pal4::ThrowPropertyActionTask::kVtable;
    ExpectFailure(r, pal4::ThrowPropertyActionTask::Create,
                  "reset_name:4496");
    Check(Index(r, "reset_name:4496") <
              Index(r, "destroy_generic:4424") &&
              Index(r, "destroy_generic:4424") <
                  Index(r, "destroy_throw"),
          "throw final failure unwinds tail in reverse order");

    r = Runtime{};
    r.expected_size = pal4::PropertyActionTask::kSize;
    r.fail_name_at = kObject + 232u;
    ExpectFailure(r, pal4::PropertyActionTask::Create,
                  "destroy_message");
    r = Runtime{};
    r.expected_size = pal4::PropertyActionTask::kSize;
    r.fail_base_at = kObject + 240u;
    ExpectFailure(r, pal4::PropertyActionTask::Create,
                  "reset_name:4328");
    r = Runtime{};
    r.expected_size = pal4::PropertyActionTask::kSize;
    r.fail_name_at = kObject + 312u;
    ExpectFailure(r, pal4::PropertyActionTask::Create,
                  "destroy_generic:4336");
    Check(Index(r, "destroy_generic:4336") <
              Index(r, "reset_name:4328"),
          "direct embedded base releases before first name");
    r = Runtime{};
    r.expected_size = pal4::PropertyActionTask::kSize;
    r.fail_write_at = kObject;
    r.fail_write_value = pal4::PropertyActionTask::kVtable;
    ExpectFailure(r, pal4::PropertyActionTask::Create,
                  "reset_name:4408");
    Check(Index(r, "reset_name:4408") <
              Index(r, "destroy_generic:4336"),
          "direct final failure unwinds second name first");

    r = Runtime{};
    r.expected_size = pal4::UsePropertyTask::kSize;
    r.allocation = 0u;
    Check(pal4::UsePropertyTask::Create(r) == 0u &&
              r.calls == std::vector<std::string>{"allocate"},
          "standalone allocation failure");
    r = Runtime{};
    r.expected_size = pal4::UsePropertyTask::kSize;
    r.fail_base_at = kObject;
    bool caught = false;
    try { (void)pal4::UsePropertyTask::Create(r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && r.calls == std::vector<std::string>({
              "allocate", "base:4096", "free"}),
          "standalone base failure frees raw allocation");
    r = Runtime{};
    r.expected_size = pal4::UsePropertyTask::kSize;
    r.fail_name_at = kObject + 72u;
    ExpectFailure(r, pal4::UsePropertyTask::Create,
                  "destroy_generic:4096");
    r = Runtime{};
    r.expected_size = pal4::UsePropertyTask::kSize;
    r.fail_write_at = kObject;
    r.fail_write_value = pal4::UsePropertyTask::kVtable;
    ExpectFailure(r, pal4::UsePropertyTask::Create,
                  "destroy_generic:4096");
}

}  // namespace

int main() {
    try {
        SuccessLayouts();
        CommonFailures();
        TailFailures();
        std::cout << "Combat property action creation tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
