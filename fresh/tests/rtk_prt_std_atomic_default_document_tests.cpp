#include "pal4/rtk_prt_std_atomic_default_document.h"

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
    std::uint32_t bits = 0u;
    std::string text;
    bool operator==(const Attribute&) const = default;
};
Attribute S(std::string key, std::string text) {
    return {std::move(key), Kind::String, 0u, std::move(text)};
}
Attribute I(std::string key, std::int32_t value) {
    return {std::move(key), Kind::Int, static_cast<std::uint32_t>(value), {}};
}
Attribute F(std::string key, float value) {
    return {std::move(key), Kind::Float, std::bit_cast<std::uint32_t>(value), {}};
}

struct Runtime final : pal4::IRtkPrtStdAtomicDefaultDocumentRuntime {
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
        Check(address == 0x1000u, "document allocation passed to constructor");
        ++initialized;
        if (fail_document) throw std::runtime_error("document");
        return 0x1100u;
    }
    void SetName(std::uint32_t target, std::string_view name,
                 std::uint32_t parent) override {
        Check(name == "RtkPrtStdAtomicObject" && parent == 0u,
              "document native class name");
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
        pal4::RtkPrtStdAtomicAttributeTemporary&, std::string_view key,
        std::string_view value) override {
        pending = S(std::string(key), std::string(value));
        return 0x4000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::RtkPrtStdAtomicAttributeTemporary&, std::string_view key,
        std::int32_t value) override {
        pending = I(std::string(key), value);
        return 0x4000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::RtkPrtStdAtomicAttributeTemporary&, std::string_view key,
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
    void DestroyAttribute(pal4::RtkPrtStdAtomicAttributeTemporary&) override {
        ++destroys;
    }
};

std::vector<Attribute> Expected() {
    return {
        S("Attr_Class_Name", "RtkPrtStdAtomicObject"),
        I("Attr_Prt_MaxPrt", 100), I("Attr_Prt_PrtEmit", 20),
        I("Attr_Prt_PrtEmit_Bias", 5), F("Attr_Prt_EmitGap", 0),
        F("Attr_Prt_EmitGap_Bias", 0), I("Attr_Prt_Alpha_AutoGradual", 1),
        F("Attr_PrtSize_x", 1), F("Attr_PrtSize_y", 1),
        F("Attr_Prt_2DRotate", 0), F("Attr_Prt_PrtLife", 1),
        F("Attr_Prt_PrtLife_Bias", 1), F("Attr_Prt_InitVel", 1),
        F("Attr_Prt_initVel_Bias", 0), F("Attr_Prt_InitDir_x", 0),
        F("Attr_Prt_InitDir_y", 1), F("Attr_Prt_InitDir_z", 0),
        F("Attr_Prt_InitDir_Bias_x", 0), F("Attr_Prt_InitDir_Bias_y", 0),
        F("Attr_Prt_InitDir_Bias_z", 0), I("Attr_Prt_Color_Red", 255),
        I("Attr_Prt_Color_Green", 255), I("Attr_Prt_Color_Blue", 255),
        I("Attr_Prt_Color_Alpha", 255), F("Attr_Prt_Gravitate", 0),
        F("Attr_Prt_Gravitate_Pos_x", 0), F("Attr_Prt_Gravitate_Pos_y", 0),
        F("Attr_Prt_Gravitate_Pos_z", 0), F("Attr_Prt_Force_x", 0),
        I("Attr_Prt_ColorGradual", 1), I("Attr_Prt_StartCol_Red", 255),
        I("Attr_Prt_StartCol_Green", 255), I("Attr_Prt_StartCol_Blue", 255),
        I("Attr_Prt_StartCol_Alpha", 255), I("Attr_Prt_StartColBias_Red", 0),
        I("Attr_Prt_StartColBias_Green", 0), I("Attr_Prt_StartColBias_Blue", 0),
        I("Attr_Prt_StartColBias_Alpha", 0), I("Attr_Prt_EndCol_Red", 0),
        I("Attr_Prt_EndCol_Green", 0), I("Attr_Prt_EndCol_Blue", 0),
        I("Attr_Prt_EndCol_Alpha", 0), I("Attr_Prt_EndColBias_Red", 0),
        I("Attr_Prt_EndColBias_Green", 0), I("Attr_Prt_EndColBias_Blue", 0),
        I("Attr_Prt_EndColBias_Alpha", 0), I("Attr_Prt_SizeGradual", 1),
        F("Attr_Prt_StartSize_x", 0), F("Attr_Prt_StartSize_y", 0),
        F("Attr_Prt_StartSizeBias_x", 0), F("Attr_Prt_StartSizeBias_y", 0),
        F("Attr_Prt_EndSize_x", 16), F("Attr_Prt_EndSize_y", 16),
        F("Attr_Prt_EndSizeBias_x", 0), F("Attr_Prt_EndSizeBias_y", 0),
        I("Attr_Prt_2DRotateGradual", 0), F("Attr_Prt_Start2DRotate", 10),
        F("Attr_Prt_Start2DRotateBias", 2), F("Attr_Prt_End2DRotate", 0),
        F("Attr_Prt_End2DRotateBias", 0), F("Attr_Prt_Atomic_Rotate_x", 0),
        F("Attr_Prt_Atomic_Rotate_y", 0), F("Attr_Prt_Atomic_Rotate_z", 0),
        F("Attr_Prt_Atomic_Scale_x", 1), F("Attr_Prt_Atomic_Scale_y", 1),
        F("Attr_Prt_Atomic_Scale_z", 1), F("Attr_Prt_Atomic_Position_x", 0),
        F("Attr_Prt_Atomic_Position_y", 0), F("Attr_Prt_Atomic_Position_z", 0),
        S("Attr_Prt_EmitterType", "\xB3\xA4\xB7\xBD\xCC\xE5"),
        F("Attr_Prt_Size_x", 10), F("Attr_Prt_Size_y", 10),
        F("Attr_Prt_Size_z", 10), S("Attr_Prt_Texture", "gloss_64"),
        F("Attr_PreRender_Time", 0)};
}

void TestDefaults() {
    const auto expected = Expected();
    Check(expected.size() == 75u, "IDA attribute count");
    Runtime fresh;
    Check(pal4::RtkPrtStdAtomicObjectDefaultDocument::Create(fresh) == 0x1100u &&
              fresh.document == 0x1100u && fresh.initialized == 1u &&
              fresh.finds == 75u && fresh.adds == 75u &&
              fresh.destroys == 75u && fresh.attributes == expected,
          "new document has all native keys, types, values and order");

    Runtime existing;
    existing.existing = true;
    Check(pal4::RtkPrtStdAtomicObjectDefaultDocument::Create(existing) == 0x1100u &&
              existing.attributes == expected && existing.finds == 75u &&
              existing.adds == 0u && existing.destroys == 0u,
          "existing attributes use typed setters without temporaries");
}

void TestFailures() {
    Runtime document_failure;
    document_failure.fail_document = true;
    try {
        (void)pal4::RtkPrtStdAtomicObjectDefaultDocument::Create(document_failure);
        Check(false, "document initialization should throw");
    } catch (const std::runtime_error&) {
        Check(document_failure.frees == 1u && document_failure.finds == 0u,
              "failed document initialization frees allocation");
    }
    for (const std::uint32_t fail_at : {1u, 30u, 31u, 75u}) {
        Runtime add_failure;
        add_failure.fail_add_at = fail_at;
        try {
            (void)pal4::RtkPrtStdAtomicObjectDefaultDocument::Create(add_failure);
            Check(false, "attribute insertion should throw");
        } catch (const std::runtime_error&) {
            Check(add_failure.adds == fail_at &&
                      add_failure.destroys == fail_at &&
                      add_failure.attributes.size() == fail_at - 1u,
                  "insertion failure destroys active temporary exactly once");
        }
    }
    Runtime no_memory;
    no_memory.allocation = 0u;
    no_memory.existing = true;
    Check(pal4::RtkPrtStdAtomicObjectDefaultDocument::Create(no_memory) == 0u &&
              no_memory.initialized == 0u && no_memory.document == 0u &&
              no_memory.attributes == Expected(),
          "allocation failure follows native null-document path");
}

}  // namespace

int main() {
    TestDefaults();
    TestFailures();
}
