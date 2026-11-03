#include "pal4/sequence_anim_default_document.h"

#include <bit>
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
    std::uint32_t bits;
    std::string text;
    bool operator==(const Attribute&) const = default;
};
Attribute S(std::string key, std::string value) {
    return {std::move(key), Kind::String, 0u, std::move(value)};
}
Attribute I(std::string key, std::int32_t value) {
    return {std::move(key), Kind::Int, static_cast<std::uint32_t>(value), {}};
}
Attribute F(std::string key, float value) {
    return {std::move(key), Kind::Float, std::bit_cast<std::uint32_t>(value), {}};
}

struct Runtime final : pal4::ISequenceAnimDefaultDocumentRuntime {
    std::vector<Attribute> attributes;
    Attribute pending{};
    std::string last_key;
    std::uint32_t allocation = 0x1000u;
    std::uint32_t document = 0xffffffffu;
    std::uint32_t initialized = 0u;
    std::uint32_t frees = 0u;
    std::uint32_t finds = 0u;
    std::uint32_t adds = 0u;
    std::uint32_t destroys = 0u;
    std::uint32_t fail_add_at = 0u;
    bool existing = false;
    bool fail_document = false;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x3cu, "document allocation size");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x1000u, "document allocation cleanup address");
        ++frees;
    }
    std::uint32_t InitializeDocument(std::uint32_t address) override {
        Check(address == 0x1000u, "document constructor address");
        ++initialized;
        if (fail_document) throw std::runtime_error("document");
        return 0x1100u;
    }
    void SetName(std::uint32_t target, std::string_view name,
                 std::uint32_t parent) override {
        Check(name == "SequnceAnimObject" && parent == 0u,
              "native document class name");
        document = target;
    }
    std::uint32_t FindAttribute(std::uint32_t target,
                                std::string_view key) override {
        Check(target == document, "attribute document handle");
        last_key = key;
        ++finds;
        return existing ? 0x3000u : 0u;
    }
    void SetString(std::uint32_t attribute, std::string_view value) override {
        Check(attribute == 0x3000u, "existing string handle");
        attributes.push_back(S(last_key, std::string(value)));
    }
    void SetInt(std::uint32_t attribute, std::int32_t value) override {
        Check(attribute == 0x3000u, "existing int handle");
        attributes.push_back(I(last_key, value));
    }
    void SetFloat(std::uint32_t attribute, float value) override {
        Check(attribute == 0x3000u, "existing float handle");
        attributes.push_back(F(last_key, value));
    }
    std::uint32_t InitializeStringAttribute(
        pal4::SequenceAnimAttributeTemporary&, std::string_view key,
        std::string_view value) override {
        pending = S(std::string(key), std::string(value));
        return 0x4000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::SequenceAnimAttributeTemporary&, std::string_view key,
        std::int32_t value) override {
        pending = I(std::string(key), value);
        return 0x4000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::SequenceAnimAttributeTemporary&, std::string_view key,
        float value) override {
        pending = F(std::string(key), value);
        return 0x4000u;
    }
    void AddAttribute(std::uint32_t target, std::uint32_t attribute) override {
        Check(target == document && attribute == 0x4000u,
              "new attribute target");
        ++adds;
        if (adds == fail_add_at) throw std::runtime_error("add");
        attributes.push_back(pending);
    }
    void DestroyAttribute(pal4::SequenceAnimAttributeTemporary&) override {
        ++destroys;
    }
};

const std::vector<Attribute> kExpected{
    S("Attr_Class_Name", "SequnceAnimObject"),
    S("Attr_SequenceAnim_Texture", "SequenceAnim\\baozha6 A.png"),
    I("Attr_Auto_AlphaBlend", 1),
    I("Attr_SequenceAnim_FPS", 60),
    I("Attr_SequenceAnim_AxisBillbord", 0),
    F("Attr_SequenceAnim_SubTexWidth", 64.0f),
    F("Attr_SequenceAnim_SubTexHeight", 64.0f),
    F("Attr_SequenceAnim_Scale_SubTexWidth", 1.0f),
    F("Attr_SequenceAnim_Scale_SubTexHeight", 1.0f),
    F("Attr_SequenceAnim_Rotate_x", 0.0f),
    F("Attr_SequenceAnim_Position_X", 0.0f),
    F("Attr_SequenceAnim_Position_Y", 0.0f),
    F("Attr_SequenceAnim_Position_Z", 0.0f),
    I("Attr_SequenceAnim_Color_Red", 255),
    I("Attr_SequenceAnim_Color_Green", 255),
    I("Attr_SequenceAnim_Color_Blue", 255),
    I("Attr_SequenceAnim_Color_Alpha", 255),
    I("Attr_Render_Last", 1),
};

}  // namespace

int main() {
    Runtime fresh;
    Check(pal4::SequenceAnimObjectDefaultDocument::Create(fresh) == 0x1100u,
          "fresh document handle");
    Check(fresh.attributes == kExpected, "fresh ordered typed defaults");
    Check(fresh.finds == 18u && fresh.adds == 18u && fresh.destroys == 18u,
          "fresh attribute lifetime");

    Runtime existing;
    existing.existing = true;
    Check(pal4::SequenceAnimObjectDefaultDocument::Create(existing) == 0x1100u,
          "existing document handle");
    Check(existing.attributes == kExpected && existing.finds == 18u &&
              existing.adds == 0u && existing.destroys == 0u,
          "existing ordered typed defaults");

    Runtime document_failure;
    document_failure.fail_document = true;
    try {
        (void)pal4::SequenceAnimObjectDefaultDocument::Create(document_failure);
        Check(false, "document exception expected");
    } catch (const std::runtime_error&) {
        Check(document_failure.frees == 1u, "constructor failure frees memory");
    }

    for (std::uint32_t fail_at : {1u, 10u, 18u}) {
        Runtime add_failure;
        add_failure.fail_add_at = fail_at;
        try {
            (void)pal4::SequenceAnimObjectDefaultDocument::Create(add_failure);
            Check(false, "attribute exception expected");
        } catch (const std::runtime_error&) {
            Check(add_failure.adds == fail_at &&
                      add_failure.destroys == fail_at,
                  "failed temporary is destroyed");
        }
    }

    Runtime no_memory;
    no_memory.allocation = 0u;
    Check(pal4::SequenceAnimObjectDefaultDocument::Create(no_memory) == 0u &&
              no_memory.initialized == 0u && no_memory.document == 0u &&
              no_memory.attributes == kExpected,
          "null allocation follows native document path");
}
