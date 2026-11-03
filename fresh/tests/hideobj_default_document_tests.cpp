#include "pal4/hideobj_default_document.h"

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

struct Runtime final : pal4::IHideobjDefaultDocumentRuntime {
    std::vector<std::string> keys;
    std::vector<std::string> values;
    std::vector<bool> tree;
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
        Check(address == 0x1000u, "failed initializer frees memory");
        ++freed;
    }
    std::uint32_t InitializeDocument(const std::uint32_t address) override {
        Check(address == 0x1000u, "document initializer address");
        if (fail_init)
            throw std::runtime_error("init");
        return 0x2000u;
    }
    void SetName(const std::uint32_t document,
                 const std::string_view name,
                 const std::uint32_t parent) override {
        Check(document == (memory == 0u ? 0u : 0x2000u) &&
                  name == "HideobjTask" && parent == 0u,
              "document name and parent");
    }
    std::uint32_t FindAttribute(const std::uint32_t document,
                                const std::string_view key) override {
        Check(document == (memory == 0u ? 0u : 0x2000u),
              "regular lookup uses document");
        keys.emplace_back(key);
        tree.push_back(false);
        return existing;
    }
    std::uint32_t FindTreeAttribute(const std::uint32_t document,
                                    const std::string_view key) override {
        Check(document == (memory == 0u ? 0u : 0x2000u),
              "tree lookup uses document");
        keys.emplace_back(key);
        tree.push_back(true);
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
        pal4::HideobjAttributeTemporary&, const std::string_view key,
        const std::string_view value) override {
        Check(key == "Attr_Class_Name", "class attribute key");
        values.emplace_back("string:" + std::string(value));
        return 0x3000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::HideobjAttributeTemporary&, const std::string_view,
        const std::int32_t value) override {
        values.emplace_back("int:" + std::to_string(value));
        return 0x3000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::HideobjAttributeTemporary&, const std::string_view,
        const float value) override {
        values.emplace_back("float:" + std::to_string(value));
        return 0x3000u;
    }
    void AddAttribute(const std::uint32_t document,
                      const std::uint32_t attribute) override {
        Check(document == (memory == 0u ? 0u : 0x2000u) &&
                  attribute == 0x3000u,
              "attribute insert arguments");
        ++added;
        if (added == fail_add_at)
            throw std::runtime_error("add");
    }
    void DestroyAttribute(pal4::HideobjAttributeTemporary&) override {
        ++destroyed;
    }
};

void CheckDefaults(const Runtime& runtime) {
    Check(runtime.keys == std::vector<std::string>{
                              "Attr_Class_Name", "Attr_Task_Wait_Mode",
                              "Attr_Task_Wait_Time", "Attr_Task_Expect_Time"},
          "four XML keys in IDA order");
    Check(runtime.values == std::vector<std::string>{
                                "string:HideobjTask", "int:1",
                                "float:0.000000", "float:0.000000"},
          "four typed defaults");
    Check(runtime.tree == std::vector<bool>{true, true, true, false},
          "first three keys use tree lookup, fourth regular lookup");
}

}  // namespace

int main() {
    Runtime fresh;
    Check(pal4::HideobjDefaultDocument::Create(fresh) == 0x2000u &&
              fresh.added == 4u && fresh.destroyed == 4u,
          "new document constructs and cleans four attributes");
    CheckDefaults(fresh);

    Runtime existing;
    existing.existing = 0x4000u;
    Check(pal4::HideobjDefaultDocument::Create(existing) == 0x2000u &&
              existing.added == 0u && existing.destroyed == 0u,
          "existing attributes are updated in place");
    CheckDefaults(existing);

    Runtime no_memory;
    no_memory.memory = 0u;
    Check(pal4::HideobjDefaultDocument::Create(no_memory) == 0u &&
              no_memory.added == 4u,
          "null allocation retains native document flow");

    Runtime failing_add;
    failing_add.fail_add_at = 3u;
    bool caught = false;
    try {
        (void)pal4::HideobjDefaultDocument::Create(failing_add);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failing_add.added == 3u &&
              failing_add.destroyed == 3u,
          "third tree-attribute temporary is destroyed on add failure");

    Runtime failing_init;
    failing_init.fail_init = true;
    caught = false;
    try {
        (void)pal4::HideobjDefaultDocument::Create(failing_init);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failing_init.freed == 1u && failing_init.keys.empty(),
          "document initialization failure frees memory");
}
