#include "pal4/rump_shahdow_default_document.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}

struct Runtime final : pal4::IRumpShahdowDefaultDocumentRuntime {
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

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x3cu, "document allocates 60 bytes"); return memory;
    }
    void Free(std::uint32_t address) override {
        Check(address == memory, "free failed allocation"); ++freed;
    }
    std::uint32_t InitializeDocument(std::uint32_t address) override {
        Check(address == memory, "initialize allocated document");
        if (fail_init) throw std::runtime_error("init");
        return 0x2000u;
    }
    void SetName(std::uint32_t document, std::string_view name,
                 std::uint32_t parent) override {
        Check(document == (memory ? 0x2000u : 0u) &&
              name == "RumpShahdowTask" && parent == 0u, "native root");
    }
    std::uint32_t FindTreeAttribute(std::uint32_t document,
                                    std::string_view key) override {
        Check(document == (memory ? 0x2000u : 0u), "tree lookup document");
        keys.emplace_back(key); tree.push_back(true); return existing;
    }
    std::uint32_t FindAttribute(std::uint32_t document,
                                std::string_view key) override {
        Check(document == (memory ? 0x2000u : 0u), "ordinary lookup document");
        keys.emplace_back(key); tree.push_back(false); return existing;
    }
    void SetString(std::uint32_t attribute, std::string_view value) override {
        Check(attribute == existing, "existing string");
        values.emplace_back("string:" + std::string(value));
    }
    void SetInt(std::uint32_t attribute, std::int32_t value) override {
        Check(attribute == existing, "existing integer");
        values.emplace_back("int:" + std::to_string(value));
    }
    void SetFloat(std::uint32_t attribute, float value) override {
        Check(attribute == existing, "existing float");
        values.emplace_back("float:" + std::to_string(value));
    }
    std::uint32_t InitializeStringAttribute(
        pal4::RumpShahdowAttributeTemporary&, std::string_view key,
        std::string_view value) override {
        Check(key == "Attr_Class_Name", "class attribute");
        values.emplace_back("string:" + std::string(value)); return 0x3000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::RumpShahdowAttributeTemporary&, std::string_view,
        std::int32_t value) override {
        values.emplace_back("int:" + std::to_string(value)); return 0x3000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::RumpShahdowAttributeTemporary&, std::string_view,
        float value) override {
        values.emplace_back("float:" + std::to_string(value)); return 0x3000u;
    }
    void AddAttribute(std::uint32_t document, std::uint32_t attribute) override {
        Check(document == (memory ? 0x2000u : 0u) && attribute == 0x3000u,
              "insert temporary");
        if (++added == fail_add_at) throw std::runtime_error("add");
    }
    void DestroyAttribute(pal4::RumpShahdowAttributeTemporary&) override {
        ++destroyed;
    }
};

void CheckDefaults(const Runtime& runtime) {
    Check(runtime.keys == std::vector<std::string>{
          "Attr_Class_Name", "Attr_Task_Wait_Mode", "Attr_Task_Wait_Time",
          "Attr_Task_Expect_Time", "Attr_Task_RumpShadow_Life",
          "Attr_Task_RumpShadow_Fps", "Attr_Task_RumpShadow_MaxCount"},
          "seven ordered XML keys");
    Check(runtime.values == std::vector<std::string>{
          "string:RumpShahdowTask", "int:2", "float:0.000000",
          "float:0.000000", "float:0.500000", "float:24.000000",
          "int:5"}, "typed XML defaults");
    Check(runtime.tree == std::vector<bool>{
          true, true, false, false, true, false, false},
          "tree lookup for class, wait mode and life");
}

}  // namespace

int main() {
    Runtime fresh;
    Check(pal4::RumpShahdowDefaultDocument::Create(fresh) == 0x2000u &&
          fresh.added == 7u && fresh.destroyed == 7u,
          "new document inserts seven attributes");
    CheckDefaults(fresh);

    Runtime existing;
    existing.existing = 0x4000u;
    Check(pal4::RumpShahdowDefaultDocument::Create(existing) == 0x2000u &&
          existing.added == 0u && existing.destroyed == 0u,
          "existing attributes use setters");
    CheckDefaults(existing);

    Runtime no_memory;
    no_memory.memory = 0u;
    Check(pal4::RumpShahdowDefaultDocument::Create(no_memory) == 0u &&
          no_memory.added == 7u,
          "null allocation follows native document flow");

    Runtime fail_add;
    fail_add.fail_add_at = 5u;
    bool caught = false;
    try { (void)pal4::RumpShahdowDefaultDocument::Create(fail_add); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && fail_add.destroyed == 5u,
          "insertion failure destroys temporary");

    Runtime fail_init;
    fail_init.fail_init = true;
    caught = false;
    try { (void)pal4::RumpShahdowDefaultDocument::Create(fail_init); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && fail_init.freed == 1u && fail_init.keys.empty(),
          "initialization failure frees raw allocation");
}
