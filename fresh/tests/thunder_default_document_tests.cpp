#include "pal4/thunder_default_document.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

enum class Kind { String, Int, Float };
struct Attribute final {
    std::string key;
    Kind kind;
    std::uint32_t bits = 0u;
    std::string text;
    bool direct = false;
    bool operator==(const Attribute&) const = default;
};
Attribute S(std::string key, std::string value) {
    return {std::move(key), Kind::String, 0u, std::move(value), false};
}
Attribute I(std::string key, std::int32_t value, bool direct = false) {
    return {std::move(key), Kind::Int, static_cast<std::uint32_t>(value),
            {}, direct};
}
Attribute F(std::string key, float value, bool direct = false) {
    return {std::move(key), Kind::Float, std::bit_cast<std::uint32_t>(value),
            {}, direct};
}

struct Runtime final : pal4::IThunderDefaultDocumentRuntime {
    std::vector<Attribute> attributes;
    Attribute pending{};
    std::string key;
    bool existing = false;
    bool fail_document = false;
    bool fail_add = false;
    std::uint32_t allocate_result = 0x1000u;
    std::uint32_t document = 0xffffffffu;
    std::uint32_t initializes = 0u;
    std::uint32_t frees = 0u;
    std::uint32_t finds = 0u;
    std::uint32_t destroys = 0u;

    std::uint32_t Allocate(const std::uint32_t size) override {
        Check(size == 60u, "XML document allocation size");
        return allocate_result;
    }
    void Free(const std::uint32_t address) override {
        Check(address == 0x1000u, "free failed initialization allocation");
        ++frees;
    }
    std::uint32_t InitializeDocument(const std::uint32_t address) override {
        Check(address == 0x1000u, "initialize allocated document");
        ++initializes;
        if (fail_document) throw std::runtime_error("document");
        return 0x1100u;
    }
    void SetName(const std::uint32_t value, const std::string_view name,
                 const std::uint32_t parent) override {
        Check(name == "ThunderObject" && parent == 0u,
              "ThunderObject root name");
        document = value;
    }
    std::uint32_t FindAttribute(const std::uint32_t value,
                                const std::string_view name) override {
        Check(value == document, "lookup named document");
        ++finds;
        key = name;
        return existing ? 0x3000u : 0u;
    }
    void SetString(const std::uint32_t address,
                   const std::string_view value) override {
        Check(address == 0x3000u, "existing string handle");
        attributes.push_back(S(key, std::string(value)));
    }
    void SetInt(const std::uint32_t address,
                const std::int32_t value) override {
        Check(address == 0x3000u, "existing int handle");
        attributes.push_back(I(key, value));
    }
    void SetFloat(const std::uint32_t address,
                  const float value) override {
        Check(address == 0x3000u, "existing float handle");
        attributes.push_back(F(key, value));
    }
    std::uint32_t InitializeStringAttribute(
        pal4::ThunderAttributeTemporary&, const std::string_view name,
        const std::string_view value) override {
        pending = S(std::string(name), std::string(value));
        return 0x4000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::ThunderAttributeTemporary&, const std::string_view name,
        const std::int32_t value) override {
        pending = I(std::string(name), value);
        return 0x4000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::ThunderAttributeTemporary&, const std::string_view name,
        const float value) override {
        pending = F(std::string(name), value);
        return 0x4000u;
    }
    void AddAttribute(const std::uint32_t value,
                      const std::uint32_t attribute) override {
        Check(value == document && attribute == 0x4000u,
              "add temporary to document");
        if (fail_add) throw std::runtime_error("add");
        attributes.push_back(pending);
    }
    void DestroyAttribute(pal4::ThunderAttributeTemporary&) override {
        ++destroys;
    }
    void WriteInt(const std::uint32_t value, const std::string_view name,
                  const std::int32_t number) override {
        Check(value == document, "direct int document");
        attributes.push_back(I(std::string(name), number, true));
    }
    void WriteFloat(const std::uint32_t value, const std::string_view name,
                    const float number) override {
        Check(value == document, "direct float document");
        attributes.push_back(F(std::string(name), number, true));
    }
};

std::vector<Attribute> Defaults() {
    return {
        S("Attr_Class_Name", "ThunderObject"),
        S("Attr_Thunder_Texture", "Bolt.png"),
        I("Attr_Thunder_AlphaBlend", 1),
        I("Attr_Thunder_Local_Model", 1),
        I("Attr_Thunder_Lock_Target", 1),
        I("Attr_Thunder_Size_Gradual", 0),
        S("Attr_Thunder_Target_Name", ""),
        F("Attr_Thunder_Fps", 60.0f),
        F("Attr_Thunder_MinRange", 0.02f),
        F("Attr_Thunder_MaxRange", 0.02f),
        F("Attr_Thunder_Target_x", 0.0f),
        F("Attr_Thunder_Target_y", 10.0f),
        F("Attr_Thunder_Target_z", 0.0f),
        F("Attr_Thunder_Section_Min_Width", 0.01f, true),
        F("Attr_Thunder_Section_Max_Width", 0.02f, true),
        F("Attr_Thunder_Section_Min_Height", 0.1f, true),
        F("Attr_Thunder_Section_Max_Height", 0.5f, true),
        F("Attr_Thunder_Scale_x", 1.0f, true),
        F("Attr_Thunder_Scale_y", 1.0f, true),
        F("Attr_Thunder_Scale_z", 1.0f, true),
        F("Attr_Thunder_Rotate_x", 0.0f, true),
        F("Attr_Thunder_Rotate_y", 0.0f, true),
        F("Attr_Thunder_Rotate_z", 0.0f, true),
        F("Attr_Thunder_Position_x", 0.0f, true),
        F("Attr_Thunder_Position_y", 0.0f, true),
        F("Attr_Thunder_Position_z", 0.0f, true),
        I("Attr_Thunder_Color_Red", 255, true),
        I("Attr_Thunder_Color_Green", 255, true),
        I("Attr_Thunder_Color_Blue", 255, true),
        I("Attr_Thunder_Color_Alpha", 180, true),
    };
}

void TestDocument(const bool existing) {
    Runtime runtime;
    runtime.existing = existing;
    Check(pal4::ThunderDefaultDocument::Create(runtime) == 0x1100u,
          "returns initialized document");
    Check(runtime.document == 0x1100u &&
              runtime.attributes == Defaults() && runtime.finds == 13u &&
              runtime.destroys == (existing ? 0u : 13u),
          "exact ordered typed XML defaults and temporary cleanup");
}

void TestFailures() {
    Runtime empty;
    empty.allocate_result = 0u;
    Check(pal4::ThunderDefaultDocument::Create(empty) == 0u &&
              empty.initializes == 0u && empty.document == 0u &&
              empty.attributes == Defaults(),
          "null allocation follows native null document path");

    Runtime init_failure;
    init_failure.fail_document = true;
    bool caught = false;
    try {
        (void)pal4::ThunderDefaultDocument::Create(init_failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && init_failure.frees == 1u,
          "failed document initialization frees allocation");

    Runtime add_failure;
    add_failure.fail_add = true;
    caught = false;
    try {
        (void)pal4::ThunderDefaultDocument::Create(add_failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && add_failure.destroys == 1u &&
              add_failure.attributes.empty(),
          "failed attribute insertion destroys temporary");
}

}  // namespace

int main() {
    TestDocument(false);
    TestDocument(true);
    TestFailures();
}
