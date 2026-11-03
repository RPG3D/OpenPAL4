#include "pal4/revolute_default_document.h"

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

struct Runtime final : pal4::IRevoluteDefaultDocumentRuntime {
    std::vector<std::string> keys;
    std::vector<std::string> values;
    std::uint32_t memory = 0x1000u;
    std::uint32_t existing = 0u;
    unsigned added = 0u;
    unsigned destroyed = 0u;
    unsigned written = 0u;
    unsigned freed = 0u;
    unsigned fail_add_at = 0u;
    bool fail_init = false;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x3cu, "document allocates 60 bytes"); return memory;
    }
    void Free(std::uint32_t address) override {
        Check(address == memory, "initialization frees raw memory"); ++freed;
    }
    std::uint32_t InitializeDocument(std::uint32_t address) override {
        Check(address == memory, "document initialized at allocated address");
        if (fail_init) throw std::runtime_error("init");
        return 0x2000u;
    }
    void SetName(std::uint32_t document, std::string_view name,
                 std::uint32_t parent) override {
        Check(document == (memory ? 0x2000u : 0u) &&
              name == "RevoluteTask" && parent == 0u, "root name");
    }
    std::uint32_t FindAttribute(std::uint32_t document,
                                std::string_view key) override {
        Check(document == (memory ? 0x2000u : 0u), "lookup document");
        keys.emplace_back(key); return existing;
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
        pal4::RevoluteAttributeTemporary&, std::string_view key,
        std::string_view value) override {
        Check(key == "Attr_Class_Name", "string key");
        values.emplace_back("string:" + std::string(value)); return 0x3000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::RevoluteAttributeTemporary&, std::string_view key,
        std::int32_t value) override {
        Check(key == "Attr_Task_Wait_Mode", "integer key");
        values.emplace_back("int:" + std::to_string(value)); return 0x3000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::RevoluteAttributeTemporary&, std::string_view,
        float value) override {
        values.emplace_back("float:" + std::to_string(value)); return 0x3000u;
    }
    void AddAttribute(std::uint32_t document, std::uint32_t attribute) override {
        Check(document == (memory ? 0x2000u : 0u) && attribute == 0x3000u,
              "insertion args");
        if (++added == fail_add_at) throw std::runtime_error("add");
    }
    void DestroyAttribute(pal4::RevoluteAttributeTemporary&) override {
        ++destroyed;
    }
    void WriteFloat(std::uint32_t document,
                    std::string_view key, float value) override {
        Check(document == (memory ? 0x2000u : 0u) && value == 0.0f,
              "dedicated float writer");
        ++written; keys.emplace_back(key);
        values.emplace_back("float:" + std::to_string(value));
    }
};

void CheckDefaults(const Runtime& runtime) {
    Check(runtime.keys == std::vector<std::string>{
          "Attr_Class_Name", "Attr_Task_Wait_Mode", "Attr_Task_Wait_Time",
          "Attr_Task_Expect_Time", "Attr_Task_Revolute_OriginPos_x",
          "Attr_Task_Revolute_OriginPos_y", "Attr_Task_Revolute_OriginPos_z",
          "Attr_Task_Revolute_Angle_x", "Attr_Task_Revolute_Angle_y",
          "Attr_Task_Revolute_Angle_z", "Attr_Task_Revolute_Center_Vel_x",
          "Attr_Task_Revolute_Center_Vel_y", "Attr_Task_Revolute_Center_Vel_z"},
          "thirteen XML defaults in native order");
    Check(runtime.values.size() == 13u &&
          runtime.values[0u] == "string:RevoluteTask" &&
          runtime.values[1u] == "int:2", "default types and class name");
    for (std::size_t i = 2u; i < runtime.values.size(); ++i)
        Check(runtime.values[i] == "float:0.000000", "zero float default");
}

}  // namespace

int main() {
    Runtime fresh;
    Check(pal4::RevoluteDefaultDocument::Create(fresh) == 0x2000u &&
          fresh.added == 10u && fresh.destroyed == 10u && fresh.written == 3u,
          "ten regular attributes and three float writers");
    CheckDefaults(fresh);

    Runtime existing;
    existing.existing = 0x4000u;
    Check(pal4::RevoluteDefaultDocument::Create(existing) == 0x2000u &&
          existing.added == 0u && existing.destroyed == 0u &&
          existing.written == 3u, "existing attributes use setter");
    CheckDefaults(existing);

    Runtime no_memory;
    no_memory.memory = 0u;
    Check(pal4::RevoluteDefaultDocument::Create(no_memory) == 0u &&
          no_memory.added == 10u && no_memory.written == 3u,
          "null allocation follows native flow");

    Runtime fail_add;
    fail_add.fail_add_at = 6u;
    bool caught = false;
    try { (void)pal4::RevoluteDefaultDocument::Create(fail_add); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && fail_add.destroyed == 6u && fail_add.written == 0u,
          "insertion failure destroys temporary");

    Runtime fail_init;
    fail_init.fail_init = true;
    caught = false;
    try { (void)pal4::RevoluteDefaultDocument::Create(fail_init); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && fail_init.freed == 1u && fail_init.keys.empty(),
          "initialization failure frees raw allocation");
}
