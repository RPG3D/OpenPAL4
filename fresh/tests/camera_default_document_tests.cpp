#include "pal4/camera_default_document.h"

#include <algorithm>
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

struct Runtime final : pal4::ICameraDefaultDocumentRuntime {
    std::vector<std::string> events;
    std::uint32_t memory = 0x4000u;
    std::uint32_t document = 0x5000u;
    std::string existing_key;
    std::string fail_add_key;
    std::string key;
    std::uint32_t existing = 0x6000u;
    bool fail_initialize = false;

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
            throw std::runtime_error("initialize");
        return document;
    }
    void SetName(std::uint32_t address, std::string_view name,
                 std::uint32_t parent) override {
        Check(address == document && name == "CameraObject" && parent == 0u,
              "native camera document name and parent");
        events.push_back("name");
    }
    std::uint32_t FindAttribute(std::uint32_t address,
                                std::string_view name) override {
        Check(address == document, "attribute document pointer");
        key = name;
        events.push_back("find:" + key);
        return key == existing_key ? existing : 0u;
    }
    void SetString(std::uint32_t address,
                   std::string_view value) override {
        Check(address == existing && key == "Attr_Class_Name" &&
                  value == "CameraObject", "existing string update");
        events.push_back("set-string");
    }
    void SetInt(std::uint32_t address, std::int32_t value) override {
        Check(address == existing && key == "Attr_Camera_Lock_LookAt" &&
                  value == 1, "existing integer update");
        events.push_back("set-int");
    }
    void SetFloat(std::uint32_t address, float value) override {
        Check(address == existing && key == "Attr_Camera_Pitch" &&
                  value == 45.0f, "existing float update");
        events.push_back("set-float");
    }
    std::uint32_t InitializeStringAttribute(
        pal4::CameraAttributeTemporary& temporary,
        std::string_view name, std::string_view value) override {
        Check(sizeof(temporary) == 28u && name == "Attr_Class_Name" &&
                  value == "CameraObject", "native string temporary");
        key = name;
        events.push_back("init-string");
        return 0x7000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::CameraAttributeTemporary& temporary,
        std::string_view name, std::int32_t value) override {
        Check(sizeof(temporary) == 28u &&
                  name == "Attr_Camera_Lock_LookAt" && value == 1,
              "native integer temporary");
        key = name;
        events.push_back("init-int");
        return 0x7000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::CameraAttributeTemporary& temporary,
        std::string_view name, float value) override {
        Check(sizeof(temporary) == 28u, "native float temporary");
        key = name;
        events.push_back("init-float:" + key + ":" +
                         std::to_string(value));
        return 0x7000u;
    }
    void AddAttribute(std::uint32_t address,
                      std::uint32_t attribute) override {
        Check(address == document && attribute == 0x7000u,
              "temporary insertion pointers");
        events.push_back("add:" + key);
        if (key == fail_add_key)
            throw std::runtime_error("add");
    }
    void DestroyAttribute(
        pal4::CameraAttributeTemporary&) override {
        events.push_back("destroy:" + key);
    }
    void WriteFloat(std::uint32_t address, std::string_view name,
                    float value) override {
        Check(address == document && name == "Attr_Camera_Roll" &&
                  value == 0.0f, "final roll writer");
        events.push_back("roll");
    }
};

void TestDefaultsAndOrder() {
    Runtime runtime;
    Check(pal4::CameraDefaultDocument::Create(runtime) == runtime.document,
          "returns initialized camera document");
    const std::vector<std::string> expected{
        "allocate:60", "initialize:16384", "name",
        "find:Attr_Class_Name", "init-string", "add:Attr_Class_Name",
        "destroy:Attr_Class_Name",
        "find:Attr_Camera_Lock_LookAt", "init-int",
        "add:Attr_Camera_Lock_LookAt",
        "destroy:Attr_Camera_Lock_LookAt",
        "find:Attr_Camera_Position_X",
        "init-float:Attr_Camera_Position_X:0.000000",
        "add:Attr_Camera_Position_X", "destroy:Attr_Camera_Position_X",
        "find:Attr_Camera_Position_Y",
        "init-float:Attr_Camera_Position_Y:100.000000",
        "add:Attr_Camera_Position_Y", "destroy:Attr_Camera_Position_Y",
        "find:Attr_Camera_Position_Z",
        "init-float:Attr_Camera_Position_Z:100.000000",
        "add:Attr_Camera_Position_Z", "destroy:Attr_Camera_Position_Z",
        "find:Attr_Camera_LookAt_x",
        "init-float:Attr_Camera_LookAt_x:0.000000",
        "add:Attr_Camera_LookAt_x", "destroy:Attr_Camera_LookAt_x",
        "find:Attr_Camera_LookAt_y",
        "init-float:Attr_Camera_LookAt_y:0.000000",
        "add:Attr_Camera_LookAt_y", "destroy:Attr_Camera_LookAt_y",
        "find:Attr_Camera_LookAt_z",
        "init-float:Attr_Camera_LookAt_z:0.000000",
        "add:Attr_Camera_LookAt_z", "destroy:Attr_Camera_LookAt_z",
        "find:Attr_Camera_Pitch",
        "init-float:Attr_Camera_Pitch:45.000000",
        "add:Attr_Camera_Pitch", "destroy:Attr_Camera_Pitch",
        "find:Attr_Camera_Yaw",
        "init-float:Attr_Camera_Yaw:180.000000",
        "add:Attr_Camera_Yaw", "destroy:Attr_Camera_Yaw", "roll"};
    Check(runtime.events == expected,
          "camera XML defaults preserve native attribute order and values");
}

void TestExistingAndFailures() {
    for (const std::string key : {"Attr_Class_Name",
                                  "Attr_Camera_Lock_LookAt",
                                  "Attr_Camera_Pitch"}) {
        Runtime runtime;
        runtime.existing_key = key;
        static_cast<void>(pal4::CameraDefaultDocument::Create(runtime));
        const auto setter = key == "Attr_Class_Name" ? "set-string" :
            key == "Attr_Camera_Lock_LookAt" ? "set-int" : "set-float";
        Check(std::find(runtime.events.begin(), runtime.events.end(),
                        setter) != runtime.events.end(),
              "existing attribute uses typed setter");
    }
    for (const std::string key : {"Attr_Class_Name",
                                  "Attr_Camera_Lock_LookAt",
                                  "Attr_Camera_Pitch"}) {
        Runtime runtime;
        runtime.fail_add_key = key;
        bool threw = false;
        try {
            static_cast<void>(pal4::CameraDefaultDocument::Create(runtime));
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Check(threw && runtime.events.back() == "destroy:" + key,
              "failed temporary insertion destroys the live attribute");
    }
    Runtime runtime;
    runtime.fail_initialize = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::CameraDefaultDocument::Create(runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events == std::vector<std::string>{
              "allocate:60", "initialize:16384", "free:16384"},
          "failed document initialization frees the allocated memory");
}

}  // namespace

int main() {
    TestDefaultsAndOrder();
    TestExistingAndFailures();
}
