#include "pal4/effect_rotate_default_document.h"

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

struct Runtime final : pal4::IEffectRotateDefaultDocumentRuntime {
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
        Check(bytes == 0x3cu, "60-byte document"); return memory;
    }
    void Free(std::uint32_t address) override {
        Check(address == memory, "free initialized address"); ++freed;
    }
    std::uint32_t InitializeDocument(std::uint32_t address) override {
        Check(address == memory, "initialize allocated document");
        if (fail_init) throw std::runtime_error("initialize");
        return 0x2000u;
    }
    void SetName(std::uint32_t document, std::string_view name,
                 std::uint32_t parent) override {
        Check(document == (memory ? 0x2000u : 0u) &&
              name == "RotateTask" && parent == 0u, "native XML root");
    }
    std::uint32_t FindTreeAttribute(std::uint32_t document,
                                    std::string_view key) override {
        Check(document == (memory ? 0x2000u : 0u), "tree lookup document");
        keys.emplace_back(key); tree.push_back(true); return existing;
    }
    std::uint32_t FindAttribute(std::uint32_t document,
                                std::string_view key) override {
        Check(document == (memory ? 0x2000u : 0u), "regular lookup document");
        keys.emplace_back(key); tree.push_back(false); return existing;
    }
    void SetString(std::uint32_t address, std::string_view value) override {
        Check(address == existing, "existing string attribute");
        values.emplace_back("string:" + std::string(value));
    }
    void SetInt(std::uint32_t address, std::int32_t value) override {
        Check(address == existing, "existing integer attribute");
        values.emplace_back("int:" + std::to_string(value));
    }
    void SetFloat(std::uint32_t address, float value) override {
        Check(address == existing, "existing float attribute");
        values.emplace_back("float:" + std::to_string(value));
    }
    std::uint32_t InitializeStringAttribute(
        pal4::EffectRotateAttributeTemporary&, std::string_view key,
        std::string_view value) override {
        Check(key == "Attr_Class_Name", "class attribute key");
        values.emplace_back("string:" + std::string(value)); return 0x3000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::EffectRotateAttributeTemporary&, std::string_view,
        std::int32_t value) override {
        values.emplace_back("int:" + std::to_string(value)); return 0x3000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::EffectRotateAttributeTemporary&, std::string_view,
        float value) override {
        values.emplace_back("float:" + std::to_string(value)); return 0x3000u;
    }
    void AddAttribute(std::uint32_t document, std::uint32_t attribute) override {
        Check(document == (memory ? 0x2000u : 0u) && attribute == 0x3000u,
              "add temporary attribute");
        if (++added == fail_add_at) throw std::runtime_error("add");
    }
    void DestroyAttribute(pal4::EffectRotateAttributeTemporary&) override {
        ++destroyed;
    }
};

void CheckDefaults(const Runtime& runtime) {
    Check(runtime.keys == std::vector<std::string>{
          "Attr_Class_Name", "Attr_Task_Wait_Mode", "Attr_Task_Wait_Time",
          "Attr_Task_Expect_Time", "Attr_Task_Rotate_Result",
          "Attr_Task_Rotate_x", "Attr_Task_Rotate_y", "Attr_Task_Rotate_z"},
          "ordered XML keys");
    Check(runtime.values == std::vector<std::string>{
          "string:RotateTask", "int:2", "float:0.000000",
          "float:0.000000", "int:0", "float:0.000000",
          "float:0.000000", "float:0.000000"},
          "typed native defaults");
    Check(runtime.tree == std::vector<bool>{
          true, false, false, false, true, false, false, false},
          "class and result use tree lookup");
}

}  // namespace

int main() {
    Runtime fresh;
    Check(pal4::EffectRotateDefaultDocument::Create(fresh) == 0x2000u &&
          fresh.added == 8u && fresh.destroyed == 8u,
          "new document inserts all attributes");
    CheckDefaults(fresh);

    Runtime existing;
    existing.existing = 0x4000u;
    Check(pal4::EffectRotateDefaultDocument::Create(existing) == 0x2000u &&
          existing.added == 0u && existing.destroyed == 0u,
          "existing fields use setters");
    CheckDefaults(existing);

    Runtime no_memory;
    no_memory.memory = 0u;
    Check(pal4::EffectRotateDefaultDocument::Create(no_memory) == 0u &&
          no_memory.added == 8u,
          "null allocation follows native XML flow");

    Runtime fail_add;
    fail_add.fail_add_at = 5u;
    bool caught = false;
    try { (void)pal4::EffectRotateDefaultDocument::Create(fail_add); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && fail_add.destroyed == 5u,
          "insertion failure destroys temporary");

    Runtime fail_init;
    fail_init.fail_init = true;
    caught = false;
    try { (void)pal4::EffectRotateDefaultDocument::Create(fail_init); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && fail_init.freed == 1u && fail_init.keys.empty(),
          "initialization failure frees raw allocation");
}
