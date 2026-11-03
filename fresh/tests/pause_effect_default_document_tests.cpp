#include "pal4/pause_effect_default_document.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPauseEffectDefaultDocumentRuntime {
    std::vector<std::string> keys;
    std::vector<std::string> values;
    std::vector<bool> tree;
    std::uint32_t memory = 0x1000u;
    std::uint32_t existing = 0u;
    unsigned added = 0u;
    unsigned destroyed = 0u;
    unsigned freed = 0u;
    unsigned fail_add_at = 0u;
    bool fail_init = false;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 0x3cu, "document is 60 bytes");
        return memory;
    }
    void Free(std::uint32_t address) override {
        Check(address == memory, "free failed initialization");
        ++freed;
    }
    std::uint32_t InitializeDocument(std::uint32_t address) override {
        Check(address == memory, "initialize allocated memory");
        if (fail_init) throw std::runtime_error("init");
        return 0x2000u;
    }
    void SetName(std::uint32_t document, std::string_view name,
                 std::uint32_t parent) override {
        Check(document == (memory ? 0x2000u : 0u) &&
                  name == "PauseEffectTask" && parent == 0u,
              "root name and parent");
    }
    std::uint32_t FindAttribute(std::uint32_t document,
                                std::string_view key) override {
        Check(document == (memory ? 0x2000u : 0u), "regular lookup document");
        keys.emplace_back(key);
        tree.push_back(false);
        return existing;
    }
    std::uint32_t FindTreeAttribute(std::uint32_t document,
                                    std::string_view key) override {
        Check(document == (memory ? 0x2000u : 0u), "tree lookup document");
        keys.emplace_back(key);
        tree.push_back(true);
        return existing;
    }
    void SetString(std::uint32_t attribute,
                   std::string_view value) override {
        Check(attribute == existing, "existing string address");
        values.emplace_back("string:" + std::string(value));
    }
    void SetInt(std::uint32_t attribute, std::int32_t value) override {
        Check(attribute == existing, "existing integer address");
        values.emplace_back("int:" + std::to_string(value));
    }
    void SetFloat(std::uint32_t attribute, float value) override {
        Check(attribute == existing, "existing float address");
        values.emplace_back("float:" + std::to_string(value));
    }
    std::uint32_t InitializeStringAttribute(
        pal4::PauseEffectAttributeTemporary&, std::string_view key,
        std::string_view value) override {
        Check(key == "Attr_Class_Name", "class key");
        values.emplace_back("string:" + std::string(value));
        return 0x3000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::PauseEffectAttributeTemporary&, std::string_view key,
        std::int32_t value) override {
        Check(key == "Attr_Task_Wait_Mode", "wait mode key");
        values.emplace_back("int:" + std::to_string(value));
        return 0x3000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::PauseEffectAttributeTemporary&, std::string_view,
        float value) override {
        values.emplace_back("float:" + std::to_string(value));
        return 0x3000u;
    }
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override {
        Check(document == (memory ? 0x2000u : 0u) &&
                  attribute == 0x3000u, "add temporary to document");
        ++added;
        if (added == fail_add_at) throw std::runtime_error("add");
    }
    void DestroyAttribute(pal4::PauseEffectAttributeTemporary&) override {
        ++destroyed;
    }
};

void CheckDefaults(const Runtime& runtime) {
    Check(runtime.keys == std::vector<std::string>{
                              "Attr_Class_Name", "Attr_Task_Wait_Mode",
                              "Attr_Task_Wait_Time", "Attr_Task_Expect_Time"} &&
              runtime.values == std::vector<std::string>{
                                    "string:PauseEffectTask", "int:1",
                                    "float:0.000000", "float:0.000000"} &&
              runtime.tree == std::vector<bool>{true, true, true, false},
          "four typed defaults and lookup paths in IDA order");
}

}  // namespace

int main() {
    Runtime fresh;
    Check(pal4::PauseEffectDefaultDocument::Create(fresh) == 0x2000u &&
              fresh.added == 4u && fresh.destroyed == 4u,
          "new document inserts four attributes");
    CheckDefaults(fresh);

    Runtime existing;
    existing.existing = 0x4000u;
    Check(pal4::PauseEffectDefaultDocument::Create(existing) == 0x2000u &&
              existing.added == 0u && existing.destroyed == 0u,
          "existing attributes updated directly");
    CheckDefaults(existing);

    Runtime no_memory;
    no_memory.memory = 0u;
    Check(pal4::PauseEffectDefaultDocument::Create(no_memory) == 0u &&
              no_memory.added == 4u,
          "null allocation retains native document flow");

    Runtime fail_add;
    fail_add.fail_add_at = 4u;
    bool caught = false;
    try { (void)pal4::PauseEffectDefaultDocument::Create(fail_add); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && fail_add.destroyed == 4u,
          "temporary destroyed when final insertion throws");

    Runtime fail_init;
    fail_init.fail_init = true;
    caught = false;
    try { (void)pal4::PauseEffectDefaultDocument::Create(fail_init); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && fail_init.freed == 1u && fail_init.keys.empty(),
          "failed initializer frees allocation");
}
