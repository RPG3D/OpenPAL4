#include "pal4/dynamic_strip_default_document.h"

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

enum class Kind { String, Int, Float };

struct Attribute final {
    std::string key;
    Kind kind;
    std::uint32_t word = 0u;
    std::string text;
    bool direct = false;
    bool operator==(const Attribute&) const = default;
};

Attribute S(std::string key, std::string text) {
    return {std::move(key), Kind::String, 0u, std::move(text), false};
}
Attribute I(std::string key, std::int32_t value, bool direct = false) {
    return {std::move(key), Kind::Int,
            static_cast<std::uint32_t>(value), {}, direct};
}
Attribute F(std::string key, float value, bool direct = false) {
    return {std::move(key), Kind::Float,
            std::bit_cast<std::uint32_t>(value), {}, direct};
}

struct Runtime final : pal4::IDynamicStripDefaultDocumentRuntime {
    std::vector<Attribute> attributes;
    Attribute pending{};
    std::string last_key;
    bool existing = false;
    bool fail_document = false;
    bool fail_add = false;
    std::uint32_t allocate_result = 0x1000u;
    std::uint32_t initialized = 0u;
    std::uint32_t finds = 0u;
    std::uint32_t adds = 0u;
    std::uint32_t destroys = 0u;
    std::uint32_t frees = 0u;
    std::uint32_t named_document = 0xffffffffu;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 60u, "XML document allocation size");
        return allocate_result;
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x1000u, "failed document allocation cleanup");
        ++frees;
    }
    std::uint32_t InitializeDocument(std::uint32_t address) override {
        Check(address == 0x1000u, "document memory");
        ++initialized;
        if (fail_document)
            throw std::runtime_error("document");
        return 0x1100u;
    }
    void SetName(std::uint32_t document, std::string_view name,
                 std::uint32_t parent) override {
        Check(name == "DynamicStripObject" && parent == 0u,
              "native document name and parent");
        named_document = document;
    }
    std::uint32_t FindAttribute(std::uint32_t document,
                                std::string_view key) override {
        Check(document == named_document, "find in named document");
        last_key = key;
        ++finds;
        return existing ? 0x3000u : 0u;
    }
    void SetString(std::uint32_t attribute,
                   std::string_view value) override {
        Check(attribute == 0x3000u, "existing string attribute");
        attributes.push_back(S(last_key, std::string(value)));
    }
    void SetInt(std::uint32_t attribute, std::int32_t value) override {
        Check(attribute == 0x3000u, "existing int attribute");
        attributes.push_back(I(last_key, value));
    }
    void SetFloat(std::uint32_t attribute, float value) override {
        Check(attribute == 0x3000u, "existing float attribute");
        attributes.push_back(F(last_key, value));
    }
    std::uint32_t InitializeStringAttribute(
        pal4::DynamicStripAttributeTemporary&, std::string_view key,
        std::string_view value) override {
        pending = S(std::string(key), std::string(value));
        return 0x4000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::DynamicStripAttributeTemporary&, std::string_view key,
        std::int32_t value) override {
        pending = I(std::string(key), value);
        return 0x4000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::DynamicStripAttributeTemporary&, std::string_view key,
        float value) override {
        pending = F(std::string(key), value);
        return 0x4000u;
    }
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override {
        Check(document == named_document && attribute == 0x4000u,
              "temporary attribute target");
        ++adds;
        if (fail_add)
            throw std::runtime_error("add");
        attributes.push_back(pending);
    }
    void DestroyAttribute(pal4::DynamicStripAttributeTemporary&) override {
        ++destroys;
    }
    void WriteInt(std::uint32_t document, std::string_view key,
                  std::int32_t value) override {
        Check(document == named_document, "direct int document");
        attributes.push_back(I(std::string(key), value, true));
    }
    void WriteFloat(std::uint32_t document, std::string_view key,
                    float value) override {
        Check(document == named_document, "direct float document");
        attributes.push_back(F(std::string(key), value, true));
    }
};

std::vector<Attribute> Expected() {
    return {
        S("Attr_Class_Name", "DynamicStripObject"),
        S("Attr_StripSystem_Texture", "\xCE\xB4\xC9\xE8\xD6\xC3"),
        I("Attr_StripSystem_AlphaBlend", 1),
        F("Attr_StripSystem_Life", 0.0f),
        F("Attr_StripSystem_Width", 0.0f),
        F("Attr_StripSystem_Position_x", 0.0f),
        F("Attr_StripSystem_Position_y", 0.0f),
        F("Attr_StripSystem_Position_z", 0.0f),
        F("Attr_StripSystem_Scale_x", 1.0f),
        F("Attr_StripSystem_Scale_y", 1.0f),
        F("Attr_StripSystem_Scale_z", 1.0f, true),
        F("Attr_StripSystem_Rotate_x", 0.0f, true),
        F("Attr_StripSystem_Rotate_y", 0.0f, true),
        F("Attr_StripSystem_Rotate_z", 0.0f, true),
        I("Attr_StripSystem_Color_Red", 255, true),
        I("Attr_StripSystem_Color_Green", 255, true),
        I("Attr_StripSystem_Color_Blue", 255, true),
        I("Attr_StripSystem_Color_Alpha", 255, true)};
}

void TestDefaultDocument() {
    Runtime runtime;
    Check(pal4::DynamicStripDefaultDocument::Create(runtime) == 0x1100u &&
              runtime.named_document == 0x1100u &&
              runtime.attributes == Expected() &&
              runtime.finds == 10u && runtime.adds == 10u &&
              runtime.destroys == 10u,
          "new document preserves all 18 native attributes and order");
    runtime = Runtime{};
    runtime.existing = true;
    Check(pal4::DynamicStripDefaultDocument::Create(runtime) == 0x1100u &&
              runtime.attributes == Expected() && runtime.finds == 10u &&
              runtime.adds == 0u && runtime.destroys == 0u,
          "existing attributes use typed setters");
}

void TestFailures() {
    Runtime runtime;
    runtime.fail_document = true;
    try {
        static_cast<void>(pal4::DynamicStripDefaultDocument::Create(runtime));
        Check(false, "document initialization must throw");
    } catch (const std::runtime_error&) {
        Check(runtime.frees == 1u && runtime.finds == 0u,
              "initialization failure frees document allocation");
    }
    runtime = Runtime{};
    runtime.fail_add = true;
    try {
        static_cast<void>(pal4::DynamicStripDefaultDocument::Create(runtime));
        Check(false, "attribute insertion must throw");
    } catch (const std::runtime_error&) {
        Check(runtime.finds == 1u && runtime.adds == 1u &&
                  runtime.destroys == 1u,
              "insertion failure destroys temporary attribute");
    }
    runtime = Runtime{};
    runtime.allocate_result = 0u;
    Check(pal4::DynamicStripDefaultDocument::Create(runtime) == 0u &&
              runtime.initialized == 0u && runtime.named_document == 0u &&
              runtime.attributes == Expected(),
          "null document follows native attribute path without initialization");
}

}  // namespace

int main() {
    TestDefaultDocument();
    TestFailures();
}
