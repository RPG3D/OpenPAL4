#include "pal4/play_sound3d_default_document.h"

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

struct Runtime final : pal4::IPlaySound3DDefaultDocumentRuntime {
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
        Check(size == 0x3cu, "document allocation size");
        return memory;
    }
    void Free(std::uint32_t address) override {
        Check(address == memory, "free initialized memory");
        ++freed;
    }
    std::uint32_t InitializeDocument(std::uint32_t address) override {
        Check(address == memory, "initialize allocated document");
        if (fail_init) throw std::runtime_error("init");
        return 0x2000u;
    }
    void SetName(std::uint32_t document, std::string_view name,
                 std::uint32_t parent) override {
        Check(document == (memory ? 0x2000u : 0u) &&
                  name == "PlaySound3DTask" && parent == 0u,
              "root name and parent");
    }
    std::uint32_t FindAttribute(std::uint32_t document,
                                std::string_view key) override {
        Check(document == (memory ? 0x2000u : 0u), "ordinary lookup document");
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
        pal4::PlaySound3DAttributeTemporary&, std::string_view key,
        std::string_view value) override {
        Check(key == "Attr_Class_Name", "class key");
        values.emplace_back("string:" + std::string(value));
        return 0x3000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::PlaySound3DAttributeTemporary&, std::string_view key,
        std::int32_t value) override {
        Check(key == "Attr_Task_Wait_Mode" ||
                  key == "Attr_Task_PlaySound3D_Loop", "integer key");
        values.emplace_back("int:" + std::to_string(value));
        return 0x3000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::PlaySound3DAttributeTemporary&, std::string_view,
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
    void DestroyAttribute(pal4::PlaySound3DAttributeTemporary&) override {
        ++destroyed;
    }
};

void CheckDefaults(const Runtime& runtime) {
    Check(runtime.keys == std::vector<std::string>{
                              "Attr_Class_Name", "Attr_Task_Wait_Mode",
                              "Attr_Task_Wait_Time", "Attr_Task_Expect_Time",
                              "Attr_Task_PlaySound3D_Loop"} &&
              runtime.values == std::vector<std::string>{
                                    "string:PlaySound3DTask", "int:1",
                                    "float:0.000000", "float:0.000000",
                                    "int:0"} &&
              runtime.tree == std::vector<bool>{true, true, true, false, true},
          "five defaults preserve IDA order, values and lookup paths");
}

}  // namespace

int main() {
    Runtime fresh;
    Check(pal4::PlaySound3DDefaultDocument::Create(fresh) == 0x2000u &&
              fresh.added == 5u && fresh.destroyed == 5u,
          "new document inserts five attributes");
    CheckDefaults(fresh);

    Runtime existing;
    existing.existing = 0x4000u;
    Check(pal4::PlaySound3DDefaultDocument::Create(existing) == 0x2000u &&
              existing.added == 0u && existing.destroyed == 0u,
          "existing attributes updated directly");
    CheckDefaults(existing);

    Runtime no_memory;
    no_memory.memory = 0u;
    Check(pal4::PlaySound3DDefaultDocument::Create(no_memory) == 0u &&
              no_memory.added == 5u,
          "null allocation retains native document flow");

    Runtime fail_add;
    fail_add.fail_add_at = 5u;
    bool caught = false;
    try { (void)pal4::PlaySound3DDefaultDocument::Create(fail_add); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && fail_add.destroyed == 5u,
          "last temporary destroyed on insertion failure");

    Runtime fail_init;
    fail_init.fail_init = true;
    caught = false;
    try { (void)pal4::PlaySound3DDefaultDocument::Create(fail_init); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && fail_init.freed == 1u && fail_init.keys.empty(),
          "initializer failure frees raw allocation");
}
