#include "pal4/move_to_target_default_document.h"

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

struct Runtime final : pal4::IMoveToTargetDefaultDocumentRuntime {
    std::vector<std::string> keys;
    std::vector<std::string> values;
    std::uint32_t memory = 0x1000u;
    std::uint32_t existing = 0u;
    unsigned added = 0u;
    unsigned destroyed = 0u;
    unsigned freed = 0u;
    unsigned written = 0u;
    unsigned fail_add_at = 0u;
    bool fail_init = false;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x3cu, "XML document allocation size");
        return memory;
    }
    void Free(std::uint32_t address) override {
        Check(address == memory, "free failed document memory");
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
                  name == "MoveToTargetTask" && parent == 0u,
              "root document name");
    }
    std::uint32_t FindAttribute(std::uint32_t document,
                                std::string_view key) override {
        Check(document == (memory ? 0x2000u : 0u),
              "regular lookup document");
        keys.emplace_back(key);
        return existing;
    }
    void SetString(std::uint32_t attribute,
                   std::string_view value) override {
        Check(attribute == existing, "existing string attribute");
        values.emplace_back("string:" + std::string(value));
    }
    void SetInt(std::uint32_t attribute, std::int32_t value) override {
        Check(attribute == existing, "existing int attribute");
        values.emplace_back("int:" + std::to_string(value));
    }
    void SetFloat(std::uint32_t attribute, float value) override {
        Check(attribute == existing, "existing float attribute");
        values.emplace_back("float:" + std::to_string(value));
    }
    std::uint32_t InitializeStringAttribute(
        pal4::MoveToTargetAttributeTemporary&, std::string_view,
        std::string_view value) override {
        values.emplace_back("string:" + std::string(value));
        return 0x3000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::MoveToTargetAttributeTemporary&, std::string_view,
        std::int32_t value) override {
        values.emplace_back("int:" + std::to_string(value));
        return 0x3000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::MoveToTargetAttributeTemporary&, std::string_view,
        float value) override {
        values.emplace_back("float:" + std::to_string(value));
        return 0x3000u;
    }
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override {
        Check(document == (memory ? 0x2000u : 0u) &&
                  attribute == 0x3000u, "add initialized attribute");
        ++added;
        if (added == fail_add_at) throw std::runtime_error("add");
    }
    void DestroyAttribute(
        pal4::MoveToTargetAttributeTemporary&) override {
        ++destroyed;
    }
    void WriteFloat(std::uint32_t document,
                    std::string_view key, float value) override {
        Check(document == (memory ? 0x2000u : 0u) &&
                  key == "Attr_Task_Target_Rotate_z" && value == 0.0f,
              "last rotation uses dedicated float writer");
        ++written;
        keys.emplace_back(key);
        values.emplace_back("float:" + std::to_string(value));
    }
};

void CheckDefaults(const Runtime& runtime) {
    Check(runtime.keys == std::vector<std::string>{
                              "Attr_Class_Name", "Attr_Task_Wait_Mode",
                              "Attr_Task_Wait_Time", "Attr_Task_Expect_Time",
                              "Attr_Task_Target_Name",
                              "Attr_Task_Target_Position_x",
                              "Attr_Task_Target_Position_y",
                              "Attr_Task_Target_Position_z",
                              "Attr_Task_Target_Rotate_x",
                              "Attr_Task_Target_Rotate_y",
                              "Attr_Task_Target_Rotate_z"},
          "eleven ordered XML keys");
    Check(runtime.values == std::vector<std::string>{
                                "string:MoveToTargetTask", "int:2",
                                "float:0.000000", "float:0.000000",
                                "string:", "float:0.000000", "float:0.000000",
                                "float:0.000000", "float:0.000000",
                                "float:0.000000", "float:0.000000"},
          "typed XML defaults include empty target name");
}

}  // namespace

int main() {
    Runtime fresh;
    Check(pal4::MoveToTargetDefaultDocument::Create(fresh) == 0x2000u &&
              fresh.added == 10u && fresh.destroyed == 10u &&
              fresh.written == 1u,
          "ten ordinary attributes and one float writer");
    CheckDefaults(fresh);

    Runtime existing;
    existing.existing = 0x4000u;
    Check(pal4::MoveToTargetDefaultDocument::Create(existing) == 0x2000u &&
              existing.added == 0u && existing.destroyed == 0u &&
              existing.written == 1u,
          "existing attributes set without temporary constructors");
    CheckDefaults(existing);

    Runtime no_memory;
    no_memory.memory = 0u;
    Check(pal4::MoveToTargetDefaultDocument::Create(no_memory) == 0u &&
              no_memory.added == 10u && no_memory.written == 1u,
          "null allocation follows native document flow");

    Runtime fail_add;
    fail_add.fail_add_at = 10u;
    bool caught = false;
    try { (void)pal4::MoveToTargetDefaultDocument::Create(fail_add); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && fail_add.destroyed == 10u && fail_add.written == 0u,
          "temporary destroyed before propagating insertion failure");

    Runtime fail_init;
    fail_init.fail_init = true;
    caught = false;
    try { (void)pal4::MoveToTargetDefaultDocument::Create(fail_init); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && fail_init.freed == 1u && fail_init.keys.empty(),
          "document initialization failure frees raw memory");
}
