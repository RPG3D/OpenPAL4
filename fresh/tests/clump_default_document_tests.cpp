#include "pal4/clump_default_document.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IClumpDefaultDocumentRuntime {
    std::vector<std::string> events;
    std::set<std::string> existing_keys;
    std::string current_key;
    std::string fail_add_key;
    std::uint32_t memory = 0x4000u;
    bool fail_initialize = false;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 60u, "document allocation size");
        events.push_back("allocate");
        return memory;
    }
    void Free(std::uint32_t address) override {
        Check(address == memory, "free allocated document memory");
        events.push_back("free");
    }
    std::uint32_t InitializeDocument(std::uint32_t address) override {
        Check(address == memory, "initialize allocated memory");
        events.push_back("initialize");
        if (fail_initialize)
            throw std::runtime_error("initialize");
        return 0x5000u;
    }
    void SetName(std::uint32_t document, std::string_view name,
                 std::uint32_t parent) override {
        Check(document == (memory == 0u ? 0u : 0x5000u) &&
                  name == "ClumpObject" && parent == 0u,
              "native document name and parent");
        events.push_back("name");
    }
    std::uint32_t FindAttribute(std::uint32_t document,
                                std::string_view key) override {
        Check(document == (memory == 0u ? 0u : 0x5000u),
              "attribute document pointer");
        current_key = key;
        events.push_back("find:" + current_key);
        return existing_keys.contains(current_key) ? 0x6000u : 0u;
    }
    void SetString(std::uint32_t attribute,
                   std::string_view value) override {
        Check(attribute == 0x6000u && current_key == "Attr_Class_Name" &&
                  value == "ClumpObject", "existing class attribute");
        events.push_back("set-string:" + current_key);
    }
    void SetFloat(std::uint32_t attribute, float value) override {
        Check(attribute == 0x6000u, "existing float attribute");
        events.push_back("set-float:" + current_key + ":" +
                         std::to_string(value));
    }
    std::uint32_t InitializeStringAttribute(
        pal4::ClumpAttributeTemporary& temporary,
        std::string_view key, std::string_view value) override {
        Check(sizeof(temporary) == 28u && key == "Attr_Class_Name" &&
                  value == "ClumpObject", "class string temporary");
        events.push_back("init-string:" + std::string(key));
        return 0x7000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::ClumpAttributeTemporary& temporary,
        std::string_view key, float value) override {
        Check(sizeof(temporary) == 28u && current_key == key,
              "float attribute temporary");
        events.push_back("init-float:" + std::string(key) + ":" +
                         std::to_string(value));
        return 0x7000u;
    }
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override {
        Check(document == (memory == 0u ? 0u : 0x5000u) &&
                  attribute == 0x7000u,
              "temporary attribute pointer");
        events.push_back("add:" + current_key);
        if (current_key == fail_add_key)
            throw std::runtime_error("add");
    }
    void DestroyAttribute(pal4::ClumpAttributeTemporary&) override {
        events.push_back("destroy:" + current_key);
    }
    void WriteInt(std::uint32_t document, std::string_view key,
                  std::int32_t value) override {
        Check(document == (memory == 0u ? 0u : 0x5000u),
              "integer writer document");
        events.push_back("int:" + std::string(key) + ":" +
                         std::to_string(value));
    }
    void WriteFloat(std::uint32_t document, std::string_view key,
                    float value) override {
        Check(document == (memory == 0u ? 0u : 0x5000u),
              "float writer document");
        events.push_back("float:" + std::string(key) + ":" +
                         std::to_string(value));
    }
    void WriteString(std::uint32_t document, std::string_view key,
                     std::string_view value) override {
        Check(document == (memory == 0u ? 0u : 0x5000u),
              "string writer document");
        events.push_back("string:" + std::string(key) + ":" +
                         std::string(value));
    }
};

void TestDefaultsAndOrder() {
    Runtime runtime;
    Check(pal4::ClumpDefaultDocument::Create(runtime) == 0x5000u,
          "returns initialized clump document");
    std::vector<std::string> expected{
        "allocate", "initialize", "name",
        "find:Attr_Class_Name", "init-string:Attr_Class_Name",
        "add:Attr_Class_Name", "destroy:Attr_Class_Name"};
    constexpr std::array<std::string_view, 9> keys{
        "Attr_Clump_Scale_x", "Attr_Clump_Scale_y",
        "Attr_Clump_Scale_z", "Attr_Clump_Rotate_x",
        "Attr_Clump_Rotate_y", "Attr_Clump_Rotate_z",
        "Attr_Clump_Position_x", "Attr_Clump_Position_y",
        "Attr_Clump_Position_z"};
    for (std::size_t i = 0u; i < keys.size(); ++i) {
        const auto key = std::string(keys[i]);
        const auto value = i < 3u ? "1.000000" : "0.000000";
        expected.push_back("find:" + key);
        expected.push_back("init-float:" + key + ":" + value);
        expected.push_back("add:" + key);
        expected.push_back("destroy:" + key);
    }
    constexpr std::string_view unset = "\xCE\xB4\xC9\xE8\xD6\xC3";
    expected.insert(expected.end(), {
        "int:Attr_Clump_Color_Red:255",
        "int:Attr_Clump_Color_Green:255",
        "int:Attr_Clump_Color_Blue:255",
        "int:Attr_Clump_Color_Alpha:255",
        "string:Attr_Clump_FileName:101\\101",
        "int:Attr_Clump_NorMap_Use:0",
        "string:Attr_Clump_NorMap_Texture:" + std::string(unset),
        "float:Attr_Clump_NorMap_Coeff:1.000000",
        "int:Attr_Clump_RefMap_Use:0",
        "string:Attr_Clump_RefMap_Texture:" + std::string(unset),
        "float:ATTR_Clump_RefMap_Coeff:0.800000",
        "int:Attr_Clump_DualMap_Use:0",
        "string:Attr_Clump_DualMap_Texture:" + std::string(unset)});
    Check(runtime.events == expected,
          "all 23 native attributes retain exact order, type and values");
}

void TestExistingAndFailures() {
    Runtime runtime;
    runtime.existing_keys = {"Attr_Class_Name", "Attr_Clump_Scale_y",
                             "Attr_Clump_Position_z"};
    static_cast<void>(pal4::ClumpDefaultDocument::Create(runtime));
    Check(runtime.events[4u] == "set-string:Attr_Class_Name" &&
              runtime.events[5u] == "find:Attr_Clump_Scale_x" &&
              runtime.events[10u] ==
                  "set-float:Attr_Clump_Scale_y:1.000000",
          "existing attributes use their typed setters without temporaries");
    Check(std::find(runtime.events.begin(), runtime.events.end(),
                    "set-float:Attr_Clump_Position_z:0.000000") !=
              runtime.events.end(), "last transform existing path");

    Runtime no_memory;
    no_memory.memory = 0u;
    Check(pal4::ClumpDefaultDocument::Create(no_memory) == 0u &&
              no_memory.events[0u] == "allocate" &&
              no_memory.events[1u] == "name",
          "failed allocation passes null document to native operations");

    Runtime fail_initialize;
    fail_initialize.fail_initialize = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::ClumpDefaultDocument::Create(
            fail_initialize));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && fail_initialize.events ==
              std::vector<std::string>{"allocate", "initialize", "free"},
          "document initialization failure frees allocated memory");

    Runtime fail_add;
    fail_add.fail_add_key = "Attr_Clump_Rotate_y";
    threw = false;
    try {
        static_cast<void>(pal4::ClumpDefaultDocument::Create(fail_add));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && fail_add.events.size() >= 2u &&
              fail_add.events[fail_add.events.size() - 2u] ==
                  "add:Attr_Clump_Rotate_y" &&
              fail_add.events.back() == "destroy:Attr_Clump_Rotate_y",
          "failed attribute insertion destroys the live temporary");
}

}  // namespace

int main() {
    TestDefaultsAndOrder();
    TestExistingAndFailures();
}
