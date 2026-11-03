#include "pal4/decress_default_document.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IDecressDefaultDocumentRuntime {
    std::vector<std::string> keys;
    std::vector<std::string> values;
    std::vector<bool> tree_lookups;
    std::uint32_t memory = 0x1000u;
    std::uint32_t existing = 0u;
    std::uint32_t added = 0u;
    std::uint32_t destroyed = 0u;
    std::uint32_t freed = 0u;
    std::uint32_t fail_add_at = 0u;
    bool fail_init = false;

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 0x3cu, "XML document allocation size");
        return memory;
    }
    void Free(const std::uint32_t address) override {
        Check(address == 0x1000u, "failed document frees allocation");
        ++freed;
    }
    std::uint32_t InitializeDocument(const std::uint32_t address) override {
        Check(address == 0x1000u, "XML initializer receives allocation");
        if (fail_init)
            throw std::runtime_error("init");
        return 0x2000u;
    }
    void SetName(const std::uint32_t document,
                 const std::string_view name,
                 const std::uint32_t parent) override {
        Check(document == (memory == 0u ? 0u : 0x2000u) &&
                  name == "DecressTask" && parent == 0u,
              "native document name and parent");
    }
    std::uint32_t FindAttribute(const std::uint32_t document,
                                const std::string_view key) override {
        Check(document == (memory == 0u ? 0u : 0x2000u),
              "regular lookup receives document");
        keys.emplace_back(key);
        tree_lookups.push_back(false);
        return existing;
    }
    std::uint32_t FindTreeAttribute(const std::uint32_t document,
                                    const std::string_view key) override {
        Check(document == (memory == 0u ? 0u : 0x2000u),
              "tree lookup receives document");
        keys.emplace_back(key);
        tree_lookups.push_back(true);
        return existing;
    }
    void SetString(const std::uint32_t attribute,
                   const std::string_view value) override {
        Check(attribute == existing, "existing string address");
        values.emplace_back("string:" + std::string(value));
    }
    void SetInt(const std::uint32_t attribute,
                const std::int32_t value) override {
        Check(attribute == existing, "existing integer address");
        values.emplace_back("int:" + std::to_string(value));
    }
    void SetFloat(const std::uint32_t attribute,
                  const float value) override {
        Check(attribute == existing, "existing float address");
        values.emplace_back("float:" + std::to_string(value));
    }
    std::uint32_t InitializeStringAttribute(
        pal4::DecressAttributeTemporary&, const std::string_view key,
        const std::string_view value) override {
        Check(key == "Attr_Class_Name", "string attribute key");
        values.emplace_back("string:" + std::string(value));
        return 0x3000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::DecressAttributeTemporary&, const std::string_view,
        const std::int32_t value) override {
        values.emplace_back("int:" + std::to_string(value));
        return 0x3000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::DecressAttributeTemporary&, const std::string_view,
        const float value) override {
        values.emplace_back("float:" + std::to_string(value));
        return 0x3000u;
    }
    void AddAttribute(const std::uint32_t document,
                      const std::uint32_t attribute) override {
        Check(document == (memory == 0u ? 0u : 0x2000u) &&
                  attribute == 0x3000u,
              "new attribute submission");
        ++added;
        if (added == fail_add_at)
            throw std::runtime_error("add");
    }
    void DestroyAttribute(pal4::DecressAttributeTemporary&) override {
        ++destroyed;
    }
};

void CheckDefaults(const Runtime& runtime) {
    Check(runtime.keys == std::vector<std::string>{
                              "Attr_Class_Name", "Attr_Task_Wait_Mode",
                              "Attr_Task_Wait_Time", "Attr_Task_Expect_Time",
                              "Attr_Task_Decress_Decress",
                              "Attr_Task_Decress_Incress",
                              "Attr_Task_Decress_Time"},
          "seven default keys in native order");
    Check(runtime.values == std::vector<std::string>{
                                "string:DecressTask", "int:2",
                                "float:0.000000", "float:0.000000",
                                "int:1", "int:0", "float:1.000000"},
          "seven defaults have correct types and values");
    Check(runtime.tree_lookups ==
              std::vector<bool>{true, true, false, false, true, false, false},
          "class, wait mode and Decress flag use tree lookup");
}

}  // namespace

int main() {
    Runtime fresh;
    Check(pal4::DecressDefaultDocument::Create(fresh) == 0x2000u &&
              fresh.added == 7u && fresh.destroyed == 7u,
          "new attributes use temporary constructors and cleanup");
    CheckDefaults(fresh);

    Runtime existing;
    existing.existing = 0x4000u;
    Check(pal4::DecressDefaultDocument::Create(existing) == 0x2000u &&
              existing.added == 0u && existing.destroyed == 0u,
          "existing attributes updated without temporary allocation");
    CheckDefaults(existing);

    Runtime no_memory;
    no_memory.memory = 0u;
    Check(pal4::DecressDefaultDocument::Create(no_memory) == 0u &&
              no_memory.added == 7u,
          "null allocation follows native document path");

    Runtime failing_add;
    failing_add.fail_add_at = 5u;
    bool caught = false;
    try {
        (void)pal4::DecressDefaultDocument::Create(failing_add);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failing_add.added == 5u &&
              failing_add.destroyed == 5u &&
              failing_add.keys[4u] == "Attr_Task_Decress_Decress",
          "special tree attribute temporary is destroyed on add failure");

    Runtime failing_init;
    failing_init.fail_init = true;
    caught = false;
    try {
        (void)pal4::DecressDefaultDocument::Create(failing_init);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failing_init.freed == 1u && failing_init.keys.empty(),
          "document initializer failure frees allocation");
}
