#include "pal4/billboard_default_document.h"

#include <cstdlib>
#include <iostream>
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

struct Runtime final : pal4::IBillboardDefaultDocumentRuntime {
    std::vector<std::string> events;
    std::uint32_t memory = 0x4000u;
    std::uint32_t document = 0x5000u;
    std::uint32_t existing = 0u;
    std::string existing_key;
    std::string fail_add_key;
    bool fail_initialize = false;
    std::string current_key;
    std::string texture;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return memory;
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
    std::uint32_t InitializeDocument(std::uint32_t address) override {
        events.push_back("initialize:" + std::to_string(address));
        if (fail_initialize)
            throw std::runtime_error("document initialization");
        return document;
    }
    void SetName(std::uint32_t address, std::string_view name,
                 std::uint32_t parent) override {
        Check(address == document && parent == 0u &&
                  name == "BillBordObject", "native document name");
        events.push_back("name:" + std::string(name));
    }
    std::uint32_t FindAttribute(std::uint32_t address,
                                std::string_view key) override {
        Check(address == document, "attribute document address");
        current_key = key;
        events.push_back("find:" + current_key);
        return key == existing_key ? existing : 0u;
    }
    void SetString(std::uint32_t address,
                   std::string_view value) override {
        Check(address == existing, "existing string address");
        events.push_back("set-string:" + current_key);
        if (current_key == "Attr_BillBord_Texture")
            texture = value;
    }
    void SetFloat(std::uint32_t address, float value) override {
        Check(address == existing, "existing float address");
        events.push_back("set-float:" + current_key + ":" +
                         std::to_string(value));
    }
    std::uint32_t InitializeStringAttribute(
        pal4::BillboardAttributeTemporary& temporary, std::string_view key,
        std::string_view value) override {
        Check(sizeof(temporary) == 28u, "native temporary size");
        current_key = key;
        events.push_back("init-string:" + current_key);
        if (key == "Attr_Class_Name")
            Check(value == "BillBordObject", "class name default");
        if (key == "Attr_BillBord_Texture")
            texture = value;
        return 0x6000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::BillboardAttributeTemporary& temporary, std::string_view key,
        float value) override {
        Check(sizeof(temporary) == 28u, "float temporary size");
        current_key = key;
        events.push_back("init-float:" + current_key + ":" +
                         std::to_string(value));
        return 0x6000u;
    }
    void AddAttribute(std::uint32_t address,
                      std::uint32_t attribute) override {
        Check(address == document && attribute == 0x6000u,
              "temporary insertion addresses");
        events.push_back("add:" + current_key);
        if (current_key == fail_add_key)
            throw std::runtime_error("attribute insertion");
    }
    void DestroyAttribute(
        pal4::BillboardAttributeTemporary&) override {
        events.push_back("destroy:" + current_key);
    }
    void WriteInt(std::uint32_t address, std::string_view key,
                  std::int32_t value) override {
        Check(address == document, "integer writer document");
        events.push_back("int:" + std::string(key) + ":" +
                         std::to_string(value));
    }
};

void TestDefaultOrderAndValues() {
    Runtime runtime;
    Check(pal4::BillboardDefaultDocument::Create(runtime) == 0x5000u,
          "returns initialized document");
    const std::vector<std::string> expected{
        "allocate:60", "initialize:16384", "name:BillBordObject",
        "find:Attr_Class_Name", "init-string:Attr_Class_Name",
        "add:Attr_Class_Name", "destroy:Attr_Class_Name",
        "find:Attr_BillBord_Texture", "init-string:Attr_BillBord_Texture",
        "add:Attr_BillBord_Texture", "destroy:Attr_BillBord_Texture",
        "find:Attr_BillBord_Width", "init-float:Attr_BillBord_Width:64.000000",
        "add:Attr_BillBord_Width", "destroy:Attr_BillBord_Width",
        "find:Attr_BillBord_Height", "init-float:Attr_BillBord_Height:64.000000",
        "add:Attr_BillBord_Height", "destroy:Attr_BillBord_Height",
        "find:Attr_BillBord_Scale_Width", "init-float:Attr_BillBord_Scale_Width:1.000000",
        "add:Attr_BillBord_Scale_Width", "destroy:Attr_BillBord_Scale_Width",
        "find:Attr_BillBord_Scale_Height", "init-float:Attr_BillBord_Scale_Height:1.000000",
        "add:Attr_BillBord_Scale_Height", "destroy:Attr_BillBord_Scale_Height",
        "find:Attr_BillBord_Rotate_x", "init-float:Attr_BillBord_Rotate_x:0.000000",
        "add:Attr_BillBord_Rotate_x", "destroy:Attr_BillBord_Rotate_x",
        "find:Attr_BillBord_Position_x", "init-float:Attr_BillBord_Position_x:0.000000",
        "add:Attr_BillBord_Position_x", "destroy:Attr_BillBord_Position_x",
        "find:Attr_BillBord_Position_y", "init-float:Attr_BillBord_Position_y:0.000000",
        "add:Attr_BillBord_Position_y", "destroy:Attr_BillBord_Position_y",
        "find:Attr_BillBord_Position_z", "init-float:Attr_BillBord_Position_z:0.000000",
        "add:Attr_BillBord_Position_z", "destroy:Attr_BillBord_Position_z",
        "int:Attr_Auto_AlphaBlend:1", "int:Attr_BillBord_AxisBillbord:1",
        "int:Attr_BillBord_Color_Red:255", "int:Attr_BillBord_Color_Green:255",
        "int:Attr_BillBord_Color_Blue:255", "int:Attr_BillBord_Color_Alpha:255",
        "int:Attr_Render_Last:1", "int:Attr_Center:1"};
    Check(runtime.events == expected, "all eighteen attributes retain native order");
    Check(runtime.texture == std::string("\xCE\xB4\xC9\xE8\xD6\xC3", 6),
          "texture fallback retains native GBK bytes");
}

void TestExistingAttributes() {
    for (const auto key : {"Attr_Class_Name", "Attr_BillBord_Texture",
                           "Attr_BillBord_Width", "Attr_BillBord_Position_z"}) {
        Runtime runtime;
        runtime.existing = 0x7000u;
        runtime.existing_key = key;
        static_cast<void>(pal4::BillboardDefaultDocument::Create(runtime));
        const std::string prefix = key == std::string_view("Attr_Class_Name") ||
                    key == std::string_view("Attr_BillBord_Texture") ?
                    "set-string:" : "set-float:";
        bool found_set = false;
        for (const auto& event : runtime.events) {
            if (event.starts_with(prefix + key))
                found_set = true;
            Check(event != "add:" + std::string(key) &&
                  event != "destroy:" + std::string(key),
                  "existing attribute bypasses temporary lifecycle");
        }
        Check(found_set, "existing attribute receives default value");
    }
}

void TestFailureCleanup() {
    Runtime runtime;
    runtime.fail_add_key = "Attr_BillBord_Width";
    try {
        static_cast<void>(pal4::BillboardDefaultDocument::Create(runtime));
        Check(false, "insertion should throw");
    } catch (const std::runtime_error&) {
        Check(runtime.events.back() == "destroy:Attr_BillBord_Width",
              "failed insertion destroys live temporary");
    }
    runtime = Runtime{};
    runtime.fail_initialize = true;
    try {
        static_cast<void>(pal4::BillboardDefaultDocument::Create(runtime));
        Check(false, "initialization should throw");
    } catch (const std::runtime_error&) {
        Check(runtime.events == std::vector<std::string>{
                  "allocate:60", "initialize:16384", "free:16384"},
              "failed document initialization frees allocation");
    }
}

}  // namespace

int main() {
    TestDefaultOrderAndValues();
    TestExistingAttributes();
    TestFailureCleanup();
}
