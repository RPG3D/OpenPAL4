#include "pal4/sound3d_default_document.h"

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
    std::string text;
    bool operator==(const Attribute&) const = default;
};

struct Runtime final : pal4::ISound3DDefaultDocumentRuntime {
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
        Check(address == 0x1000u, "failed initialization free address");
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
        Check(name == "Sound3DObject" && parent == 0u, "native document name");
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
        attributes.push_back({last_key, Kind::String, std::string(value)});
    }
    void SetInt(std::uint32_t attribute, std::int32_t value) override {
        Check(attribute == 0x3000u && value == 0, "existing int value");
        attributes.push_back({last_key, Kind::Int, {}});
    }
    void SetFloat(std::uint32_t attribute, float value) override {
        Check(attribute == 0x3000u && value == 0.0f, "existing float value");
        attributes.push_back({last_key, Kind::Float, {}});
    }
    std::uint32_t InitializeStringAttribute(
        pal4::Sound3DAttributeTemporary&, std::string_view key,
        std::string_view value) override {
        pending = {std::string(key), Kind::String, std::string(value)};
        return 0x4000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::Sound3DAttributeTemporary&, std::string_view key,
        std::int32_t value) override {
        Check(value == 0, "new int value");
        pending = {std::string(key), Kind::Int, {}};
        return 0x4000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::Sound3DAttributeTemporary&, std::string_view key,
        float value) override {
        Check(value == 0.0f, "new float value");
        pending = {std::string(key), Kind::Float, {}};
        return 0x4000u;
    }
    void AddAttribute(std::uint32_t target, std::uint32_t attribute) override {
        Check(target == document && attribute == 0x4000u,
              "new attribute target");
        ++adds;
        if (adds == fail_add_at) throw std::runtime_error("add");
        attributes.push_back(pending);
    }
    void DestroyAttribute(pal4::Sound3DAttributeTemporary&) override {
        ++destroys;
    }
};

const std::vector<Attribute> kExpected{
    {"Attr_Class_Name", Kind::String, "Sound3DObject"},
    {"Attr_Sound3D_SoundPos_x", Kind::Float, {}},
    {"Attr_Sound3D_SoundPos_y", Kind::Float, {}},
    {"Attr_Sound3D_SoundPos_z", Kind::Float, {}},
    {"Attr_Sound3D_SoundFile", Kind::String, "type.wav"},
    {"Attr_Sound3D_2D", Kind::Int, {}},
};

}  // namespace

int main() {
    Runtime fresh;
    Check(pal4::Sound3DObjectDefaultDocument::Create(fresh) == 0x1100u,
          "fresh document handle");
    Check(fresh.attributes == kExpected && fresh.finds == 6u &&
              fresh.adds == 6u && fresh.destroys == 6u,
          "fresh ordered typed defaults and temporary lifetime");

    Runtime existing;
    existing.existing = true;
    Check(pal4::Sound3DObjectDefaultDocument::Create(existing) == 0x1100u &&
              existing.attributes == kExpected && existing.finds == 6u &&
              existing.adds == 0u && existing.destroys == 0u,
          "existing attributes are overwritten in order");

    Runtime document_failure;
    document_failure.fail_document = true;
    try {
        (void)pal4::Sound3DObjectDefaultDocument::Create(document_failure);
        Check(false, "document exception expected");
    } catch (const std::runtime_error&) {
        Check(document_failure.frees == 1u,
              "constructor failure frees allocation");
    }

    for (std::uint32_t fail_at : {1u, 3u, 6u}) {
        Runtime add_failure;
        add_failure.fail_add_at = fail_at;
        try {
            (void)pal4::Sound3DObjectDefaultDocument::Create(add_failure);
            Check(false, "attribute exception expected");
        } catch (const std::runtime_error&) {
            Check(add_failure.adds == fail_at &&
                      add_failure.destroys == fail_at,
                  "failed temporary is destroyed");
        }
    }

    Runtime no_memory;
    no_memory.allocation = 0u;
    Check(pal4::Sound3DObjectDefaultDocument::Create(no_memory) == 0u &&
              no_memory.initialized == 0u && no_memory.document == 0u &&
              no_memory.attributes == kExpected,
          "null allocation follows native document path");
}
