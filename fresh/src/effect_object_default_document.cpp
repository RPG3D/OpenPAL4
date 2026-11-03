#include "pal4/effect_object_default_document.h"

namespace pal4 {
namespace {

constexpr std::string_view kObjectName = "EffectObject";

void SetStringAttribute(
    const std::uint32_t document, const std::string_view key,
    const std::string_view value,
    IEffectObjectDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetString(existing, value);
        return;
    }
    EffectObjectAttributeTemporary temporary{};
    const auto attribute = runtime.InitializeStringAttribute(
        temporary, key, value);
    try {
        runtime.AddAttribute(document, attribute);
    } catch (...) {
        runtime.DestroyAttribute(temporary);
        throw;
    }
    runtime.DestroyAttribute(temporary);
}

void SetFloatAttribute(
    const std::uint32_t document, const std::string_view key,
    const float value,
    IEffectObjectDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetFloat(existing, value);
        return;
    }
    EffectObjectAttributeTemporary temporary{};
    const auto attribute = runtime.InitializeFloatAttribute(
        temporary, key, value);
    try {
        runtime.AddAttribute(document, attribute);
    } catch (...) {
        runtime.DestroyAttribute(temporary);
        throw;
    }
    runtime.DestroyAttribute(temporary);
}

}  // namespace

std::uint32_t EffectObjectDefaultDocument::Create(
    IEffectObjectDefaultDocumentRuntime& runtime) {
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
    runtime.SetName(document, kObjectName, 0u);
    SetStringAttribute(document, "Attr_Class_Name", kObjectName, runtime);
    SetFloatAttribute(document, "Attr_EffectObject_Position_x", 0.0f, runtime);
    SetFloatAttribute(document, "Attr_EffectObject_Position_y", 0.0f, runtime);
    SetFloatAttribute(document, "Attr_EffectObject_Position_z", 0.0f, runtime);
    SetFloatAttribute(document, "Attr_EffectObject_Scale_x", 1.0f, runtime);
    SetFloatAttribute(document, "Attr_EffectObject_Scale_y", 1.0f, runtime);
    SetFloatAttribute(document, "Attr_EffectObject_Scale_z", 1.0f, runtime);
    SetFloatAttribute(document, "Attr_EffectObject_Rotate_x", 0.0f, runtime);
    SetFloatAttribute(document, "Attr_EffectObject_Rotate_y", 0.0f, runtime);
    SetFloatAttribute(document, "Attr_EffectObject_Rotate_z", 0.0f, runtime);
    return document;
}

}  // namespace pal4
