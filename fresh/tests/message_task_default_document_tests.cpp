#include "pal4/message_task_default_document.h"

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

struct Runtime final : pal4::IMessageTaskDefaultDocumentRuntime {
    std::vector<std::string> calls;
    std::uint32_t memory = 0x1000u;
    std::uint32_t existing = 0u;
    unsigned added = 0u;
    unsigned destroyed = 0u;
    unsigned freed = 0u;
    unsigned fail_add_at = 0u;
    bool fail_init = false;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 0x3cu, "document is 60 bytes");
        calls.emplace_back("allocate");
        return memory;
    }
    void Free(std::uint32_t address) override {
        Check(address == memory, "free initialized memory");
        ++freed;
    }
    std::uint32_t InitializeDocument(std::uint32_t address) override {
        Check(address == memory, "initialize allocated document");
        if (fail_init) throw std::runtime_error("initialize");
        calls.emplace_back("initialize");
        return 0x2000u;
    }
    void SetName(std::uint32_t document, std::string_view name,
                 std::uint32_t parent) override {
        Check(document == (memory ? 0x2000u : 0u) &&
                  name == "MessageTask" && parent == 0u,
              "root name and parent");
        calls.emplace_back("name");
    }
    std::uint32_t FindAttribute(std::uint32_t document,
                                std::string_view key) override {
        Check(document == (memory ? 0x2000u : 0u), "regular lookup document");
        calls.emplace_back("regular:" + std::string(key));
        return existing;
    }
    std::uint32_t FindTreeAttribute(std::uint32_t document,
                                    std::string_view key) override {
        Check(document == (memory ? 0x2000u : 0u), "tree lookup document");
        calls.emplace_back("tree:" + std::string(key));
        return existing;
    }
    void SetString(std::uint32_t address, std::string_view value) override {
        Check(address == existing, "existing string attribute");
        calls.emplace_back("string:" + std::string(value));
    }
    void SetInt(std::uint32_t address, std::int32_t value) override {
        Check(address == existing, "existing integer attribute");
        calls.emplace_back("int:" + std::to_string(value));
    }
    void SetFloat(std::uint32_t address, float value) override {
        Check(address == existing, "existing float attribute");
        calls.emplace_back("float:" + std::to_string(value));
    }
    std::uint32_t InitializeStringAttribute(
        pal4::MessageTaskAttributeTemporary&, std::string_view key,
        std::string_view value) override {
        calls.emplace_back("new:" + std::string(key) + "=" +
                           std::string(value));
        return 0x3000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::MessageTaskAttributeTemporary&, std::string_view key,
        std::int32_t value) override {
        calls.emplace_back("new:" + std::string(key) + "=" +
                           std::to_string(value));
        return 0x3000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::MessageTaskAttributeTemporary&, std::string_view key,
        float value) override {
        calls.emplace_back("new:" + std::string(key) + "=" +
                           std::to_string(value));
        return 0x3000u;
    }
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override {
        Check(document == (memory ? 0x2000u : 0u) &&
                  attribute == 0x3000u, "add initialized attribute");
        ++added;
        if (added == fail_add_at) throw std::runtime_error("add");
    }
    void DestroyAttribute(pal4::MessageTaskAttributeTemporary&) override {
        ++destroyed;
    }
};

void CheckFields(const Runtime& runtime, bool has_existing) {
    const std::string prefix = has_existing ? "" : "new:";
    const auto expected = std::vector<std::string>{
        "allocate", "initialize", "name",
        "tree:Attr_Class_Name", prefix +
            (has_existing ? "string:MessageTask" :
                            "Attr_Class_Name=MessageTask"),
        "tree:Attr_Task_Wait_Mode", prefix +
            (has_existing ? "int:2" : "Attr_Task_Wait_Mode=2"),
        "tree:Attr_Task_Wait_Time", prefix +
            (has_existing ? "float:0.000000" :
                            "Attr_Task_Wait_Time=0.000000"),
        "regular:Attr_Task_Expect_Time", prefix +
            (has_existing ? "float:0.000000" :
                            "Attr_Task_Expect_Time=0.000000"),
        "tree:Attr_Task_Message_What", prefix +
            (has_existing ? "string:message" :
                            "Attr_Task_Message_What=message")};
    Check(runtime.calls == expected,
          "five defaults retain native order, values and lookup paths");
}

}  // namespace

int main() {
    Runtime fresh;
    Check(pal4::MessageTaskDefaultDocument::Create(fresh) == 0x2000u &&
              fresh.added == 5u && fresh.destroyed == 5u,
          "missing attributes inserted and temporaries destroyed");
    CheckFields(fresh, false);

    Runtime existing;
    existing.existing = 0x4000u;
    Check(pal4::MessageTaskDefaultDocument::Create(existing) == 0x2000u &&
              existing.added == 0u && existing.destroyed == 0u,
          "existing attributes updated in place");
    CheckFields(existing, true);

    Runtime no_memory;
    no_memory.memory = 0u;
    Check(pal4::MessageTaskDefaultDocument::Create(no_memory) == 0u &&
              no_memory.added == 5u && no_memory.destroyed == 5u,
          "null allocation retains native document flow");

    Runtime fail_add;
    fail_add.fail_add_at = 5u;
    bool caught = false;
    try { (void)pal4::MessageTaskDefaultDocument::Create(fail_add); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && fail_add.added == 5u && fail_add.destroyed == 5u,
          "last temporary released when insertion throws");

    Runtime fail_init;
    fail_init.fail_init = true;
    caught = false;
    try { (void)pal4::MessageTaskDefaultDocument::Create(fail_init); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && fail_init.freed == 1u && fail_init.calls.size() == 1u,
          "failed document initializer frees raw allocation");
}
