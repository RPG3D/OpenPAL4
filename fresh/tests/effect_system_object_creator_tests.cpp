#include "pal4/effect_system_object_creator.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
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

struct Runtime final : pal4::IEffectSystemObjectCreateRuntime {
    std::optional<std::string> class_name = "ClumpObject";
    std::uint32_t factory = 0x5000u;
    std::uint32_t object = 0x6000u;
    bool init_result = true;
    std::vector<std::string> calls;

    std::optional<std::string> StringAttribute(
        std::uint32_t document, std::string_view name) override {
        Check(document == 0x1000u && name == "Attr_Class_Name",
              "class attribute reads the source XML document");
        calls.push_back("attribute");
        return class_name;
    }
    std::uint32_t FindFactory(std::string_view name) override {
        Check(name == *class_name, "factory lookup uses XML class name");
        calls.push_back("lookup");
        return factory;
    }
    std::uint32_t InvokeFactory(std::uint32_t address) override {
        Check(address == factory, "lookup result selects factory callback");
        calls.push_back("create");
        return object;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check((address == object + 12u && value == 0x2000u) ||
                  (address == object + 16u && value == 0x3000u),
              "object owner and context offsets");
        calls.push_back(address == object + 12u ? "owner" : "context");
    }
    bool InitializeObject(std::uint32_t address,
                          std::uint32_t document) override {
        Check(address == object && document == 0x1000u,
              "virtual initializer receives new object and XML");
        calls.push_back("initialize");
        return init_result;
    }
    void DestroyObject(std::uint32_t address) override {
        Check(address == object, "failed object is destroyed");
        calls.push_back("destroy");
    }
};

void TestGuards() {
    Runtime runtime;
    Check(pal4::EffectSystemObjectCreator::CreateFromDocument(
              0u, 0x2000u, 0x3000u, runtime) == 0u &&
              runtime.calls.empty(),
          "null document returns before attribute access");
    runtime.class_name.reset();
    Check(pal4::EffectSystemObjectCreator::CreateFromDocument(
              0x1000u, 0x2000u, 0x3000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"attribute"},
          "missing class attribute returns null");
    runtime.calls.clear();
    runtime.class_name = "UnknownObject";
    runtime.factory = 0u;
    Check(pal4::EffectSystemObjectCreator::CreateFromDocument(
              0x1000u, 0x2000u, 0x3000u, runtime) == 0u &&
              runtime.calls ==
                  std::vector<std::string>{"attribute", "lookup"},
          "unregistered class returns null");
    runtime.calls.clear();
    runtime.factory = 0x5000u;
    runtime.object = 0u;
    Check(pal4::EffectSystemObjectCreator::CreateFromDocument(
              0x1000u, 0x2000u, 0x3000u, runtime) == 0u &&
              runtime.calls ==
                  std::vector<std::string>{"attribute", "lookup",
                                           "create"},
          "factory allocation failure stops before object fields");
}

void TestLifecycle() {
    Runtime runtime;
    Check(pal4::EffectSystemObjectCreator::CreateFromDocument(
              0x1000u, 0x2000u, 0x3000u, runtime) == runtime.object &&
              runtime.calls ==
                  std::vector<std::string>{"attribute", "lookup",
                                           "create", "owner", "context",
                                           "initialize"},
          "successful factory sets fields before virtual initialization");
    runtime.calls.clear();
    runtime.init_result = false;
    Check(pal4::EffectSystemObjectCreator::CreateFromDocument(
              0x1000u, 0x2000u, 0x3000u, runtime) == 0u &&
              runtime.calls ==
                  std::vector<std::string>{"attribute", "lookup",
                                           "create", "owner", "context",
                                           "initialize", "destroy"},
          "failed initializer destroys the constructed object");
}

}  // namespace

int main() {
    TestGuards();
    TestLifecycle();
}
