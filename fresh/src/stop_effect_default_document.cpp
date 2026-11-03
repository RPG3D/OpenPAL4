#include "pal4/stop_effect_default_document.h"

#include <array>

namespace pal4 {
namespace {

enum class Kind { String, Int, Float };
struct Default final {
    std::string_view key;
    Kind kind;
    bool tree_lookup;
};
constexpr std::array<Default, 4u> kDefaults{{
    {"Attr_Class_Name", Kind::String, true},
    {"Attr_Task_Wait_Mode", Kind::Int, true},
    {"Attr_Task_Wait_Time", Kind::Float, true},
    {"Attr_Task_Expect_Time", Kind::Float, false},
}};

void WriteDefault(std::uint32_t document, const Default& value,
                  IStopEffectDefaultDocumentRuntime& runtime) {
    const auto existing = value.tree_lookup
        ? runtime.FindTreeAttribute(document, value.key)
        : runtime.FindAttribute(document, value.key);
    if (existing != 0u) {
        switch (value.kind) {
        case Kind::String:
            runtime.SetString(existing, "StopEffectTask"); break;
        case Kind::Int:
            runtime.SetInt(existing, 1); break;
        case Kind::Float:
            runtime.SetFloat(existing, 0.0f); break;
        }
        return;
    }
    StopEffectAttributeTemporary temporary{};
    std::uint32_t attribute = 0u;
    switch (value.kind) {
    case Kind::String:
        attribute = runtime.InitializeStringAttribute(
            temporary, value.key, "StopEffectTask"); break;
    case Kind::Int:
        attribute = runtime.InitializeIntAttribute(
            temporary, value.key, 1); break;
    case Kind::Float:
        attribute = runtime.InitializeFloatAttribute(
            temporary, value.key, 0.0f); break;
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

std::uint32_t StopEffectDefaultDocument::Create(
    IStopEffectDefaultDocumentRuntime& runtime) {
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
    runtime.SetName(document, "StopEffectTask", 0u);
    for (const auto& value : kDefaults)
        WriteDefault(document, value, runtime);
    return document;
}

}  // namespace pal4
