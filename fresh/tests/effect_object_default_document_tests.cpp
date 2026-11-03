#include "pal4/effect_object_default_document.h"

#include <bit>
#include <cstdint>
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

struct Attribute final {
    std::string key;
    std::uint32_t value = 0u;
    std::string text;
    bool operator==(const Attribute&) const = default;
};

struct Runtime final : pal4::IEffectObjectDefaultDocumentRuntime {
    std::vector<Attribute> attributes;
    Attribute pending{};
    std::string last_key;
    bool existing = false;
    bool fail_document = false;
    bool fail_add = false;
    std::uint32_t allocation = 0x1000u;
    std::uint32_t initialized = 0u;
    std::uint32_t finds = 0u;
    std::uint32_t adds = 0u;
    std::uint32_t destroys = 0u;
    std::uint32_t frees = 0u;
    std::uint32_t named_document = 0xffffffffu;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 60u, "XML document allocation size");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x1000u, "failed document cleanup address");
        ++frees;
    }
    std::uint32_t InitializeDocument(std::uint32_t address) override {
        Check(address == 0x1000u, "document storage address");
        ++initialized;
        if (fail_document)
            throw std::runtime_error("document init");
        return 0x1100u;
    }
    void SetName(std::uint32_t document, std::string_view name,
                 std::uint32_t parent) override {
        Check(name == "EffectObject" && parent == 0u,
              "native XML document name and parent");
        named_document = document;
    }
    std::uint32_t FindAttribute(std::uint32_t document,
                                std::string_view key) override {
        Check(document == named_document, "attribute belongs to document");
        last_key = key;
        ++finds;
        return existing ? 0x3000u : 0u;
    }
    void SetString(std::uint32_t attribute,
                   std::string_view value) override {
        Check(attribute == 0x3000u, "existing string attribute handle");
        attributes.push_back({last_key, 0u, std::string(value)});
    }
    void SetFloat(std::uint32_t attribute, float value) override {
        Check(attribute == 0x3000u, "existing float attribute handle");
        attributes.push_back({last_key,
                              std::bit_cast<std::uint32_t>(value), {}});
    }
    std::uint32_t InitializeStringAttribute(
        pal4::EffectObjectAttributeTemporary&, std::string_view key,
        std::string_view value) override {
        pending = {std::string(key), 0u, std::string(value)};
        return 0x4000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::EffectObjectAttributeTemporary&, std::string_view key,
        float value) override {
        pending = {std::string(key),
                   std::bit_cast<std::uint32_t>(value), {}};
        return 0x4000u;
    }
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override {
        Check(document == named_document && attribute == 0x4000u,
              "new attribute is added to named document");
        ++adds;
        if (fail_add)
            throw std::runtime_error("add attribute");
        attributes.push_back(pending);
    }
    void DestroyAttribute(
        pal4::EffectObjectAttributeTemporary&) override {
        ++destroys;
    }
};

void CheckAttributes(const std::vector<Attribute>& actual) {
    const std::vector<Attribute> expected{
        {"Attr_Class_Name", 0u, "EffectObject"},
        {"Attr_EffectObject_Position_x", 0u, {}},
        {"Attr_EffectObject_Position_y", 0u, {}},
        {"Attr_EffectObject_Position_z", 0u, {}},
        {"Attr_EffectObject_Scale_x", 0x3f800000u, {}},
        {"Attr_EffectObject_Scale_y", 0x3f800000u, {}},
        {"Attr_EffectObject_Scale_z", 0x3f800000u, {}},
        {"Attr_EffectObject_Rotate_x", 0u, {}},
        {"Attr_EffectObject_Rotate_y", 0u, {}},
        {"Attr_EffectObject_Rotate_z", 0u, {}}};
    Check(actual == expected, "all ten exact XML keys, types and defaults");
}

void TestCreate() {
    Runtime runtime;
    Check(pal4::EffectObjectDefaultDocument::Create(runtime) == 0x1100u &&
              runtime.named_document == 0x1100u &&
              runtime.initialized == 1u && runtime.finds == 10u &&
              runtime.adds == 10u && runtime.destroys == 10u,
          "new attribute path initializes and destroys each temporary");
    CheckAttributes(runtime.attributes);

    Runtime existing;
    existing.existing = true;
    Check(pal4::EffectObjectDefaultDocument::Create(existing) == 0x1100u &&
              existing.finds == 10u && existing.adds == 0u &&
              existing.destroys == 0u,
          "existing attribute path sets values directly");
    CheckAttributes(existing.attributes);
}

void TestFailure() {
    Runtime init_failure;
    init_failure.fail_document = true;
    try {
        (void)pal4::EffectObjectDefaultDocument::Create(init_failure);
        Check(false, "document initialization must throw");
    } catch (const std::runtime_error&) {
        Check(init_failure.frees == 1u,
              "failed document initialization releases allocation");
    }
    Runtime add_failure;
    add_failure.fail_add = true;
    try {
        (void)pal4::EffectObjectDefaultDocument::Create(add_failure);
        Check(false, "attribute insertion must throw");
    } catch (const std::runtime_error&) {
        Check(add_failure.adds == 1u && add_failure.destroys == 1u,
              "attribute insertion failure cleans temporary");
    }
    Runtime no_memory;
    no_memory.allocation = 0u;
    no_memory.existing = true;
    Check(pal4::EffectObjectDefaultDocument::Create(no_memory) == 0u &&
              no_memory.initialized == 0u &&
              no_memory.named_document == 0u,
          "allocation failure still follows native null-document path");
}

}  // namespace

int main() {
    TestCreate();
    TestFailure();
}
