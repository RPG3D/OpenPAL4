#include "pal4/rump_shahdow_default_document.h"

#include <array>

namespace pal4 {
namespace {

enum class Kind { String, Int, Float };

struct Default final {
    std::string_view key;
    Kind kind;
    std::int32_t integer = 0;
    float floating = 0.0f;
    bool tree_lookup = false;
};

constexpr std::array<Default, 7u> kDefaults{{
    {"Attr_Class_Name", Kind::String, 0, 0.0f, true},
    {"Attr_Task_Wait_Mode", Kind::Int, 2, 0.0f, true},
    {"Attr_Task_Wait_Time", Kind::Float},
    {"Attr_Task_Expect_Time", Kind::Float},
    {"Attr_Task_RumpShadow_Life", Kind::Float, 0, 0.5f, true},
    {"Attr_Task_RumpShadow_Fps", Kind::Float, 0, 24.0f},
    {"Attr_Task_RumpShadow_MaxCount", Kind::Int, 5},
}};

void WriteDefault(const std::uint32_t document, const Default& value,
                  IRumpShahdowDefaultDocumentRuntime& runtime) {
    const auto existing = value.tree_lookup
                              ? runtime.FindTreeAttribute(document, value.key)
                              : runtime.FindAttribute(document, value.key);
    if (existing != 0u) {
        switch (value.kind) {
        case Kind::String:
            runtime.SetString(existing, "RumpShahdowTask"); break;
        case Kind::Int: runtime.SetInt(existing, value.integer); break;
        case Kind::Float: runtime.SetFloat(existing, value.floating); break;
        }
        return;
    }
    RumpShahdowAttributeTemporary temporary{};
    std::uint32_t attribute = 0u;
    switch (value.kind) {
    case Kind::String:
        attribute = runtime.InitializeStringAttribute(
            temporary, value.key, "RumpShahdowTask");
        break;
    case Kind::Int:
        attribute = runtime.InitializeIntAttribute(
            temporary, value.key, value.integer);
        break;
    case Kind::Float:
        attribute = runtime.InitializeFloatAttribute(
            temporary, value.key, value.floating);
        break;
    }
    try {
        runtime.AddAttribute(document, attribute);
    } catch (...) {
        runtime.DestroyAttribute(temporary);
        throw;
    }
    runtime.DestroyAttribute(temporary);
}

}  // namespace

std::uint32_t RumpShahdowDefaultDocument::Create(
    IRumpShahdowDefaultDocumentRuntime& runtime) {
    const auto memory = runtime.Allocate(0x3cu);
    std::uint32_t document = 0u;
    if (memory != 0u) {
        try {
            document = runtime.InitializeDocument(memory);
        } catch (...) {
            runtime.Free(memory);
            throw;
        }
    }
    runtime.SetName(document, "RumpShahdowTask", 0u);
    for (const auto& value : kDefaults)
        WriteDefault(document, value, runtime);
    return document;
}

}  // namespace pal4
