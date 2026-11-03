#include "pal4/dynamic_strip_default_document.h"

namespace pal4 {
namespace {

constexpr std::string_view kObjectName = "DynamicStripObject";
constexpr std::string_view kUnsetTexture = "\xCE\xB4\xC9\xE8\xD6\xC3";

void SetStringAttribute(const std::uint32_t document,
                        const std::string_view key,
                        const std::string_view value,
                        IDynamicStripDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetString(existing, value);
        return;
    }
    DynamicStripAttributeTemporary temporary{};
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

void SetIntAttribute(const std::uint32_t document,
                     const std::string_view key, const std::int32_t value,
                     IDynamicStripDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetInt(existing, value);
        return;
    }
    DynamicStripAttributeTemporary temporary{};
    const auto attribute = runtime.InitializeIntAttribute(
        temporary, key, value);
    try {
        runtime.AddAttribute(document, attribute);
    } catch (...) {
        runtime.DestroyAttribute(temporary);
        throw;
    }
    runtime.DestroyAttribute(temporary);
}

void SetFloatAttribute(const std::uint32_t document,
                       const std::string_view key, const float value,
                       IDynamicStripDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetFloat(existing, value);
        return;
    }
    DynamicStripAttributeTemporary temporary{};
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

std::uint32_t DynamicStripDefaultDocument::Create(
    IDynamicStripDefaultDocumentRuntime& runtime) {
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
    SetStringAttribute(document, "Attr_StripSystem_Texture", kUnsetTexture,
                       runtime);
    SetIntAttribute(document, "Attr_StripSystem_AlphaBlend", 1, runtime);
    SetFloatAttribute(document, "Attr_StripSystem_Life", 0.0f, runtime);
    SetFloatAttribute(document, "Attr_StripSystem_Width", 0.0f, runtime);
    SetFloatAttribute(document, "Attr_StripSystem_Position_x", 0.0f, runtime);
    SetFloatAttribute(document, "Attr_StripSystem_Position_y", 0.0f, runtime);
    SetFloatAttribute(document, "Attr_StripSystem_Position_z", 0.0f, runtime);
    SetFloatAttribute(document, "Attr_StripSystem_Scale_x", 1.0f, runtime);
    SetFloatAttribute(document, "Attr_StripSystem_Scale_y", 1.0f, runtime);
    runtime.WriteFloat(document, "Attr_StripSystem_Scale_z", 1.0f);
    runtime.WriteFloat(document, "Attr_StripSystem_Rotate_x", 0.0f);
    runtime.WriteFloat(document, "Attr_StripSystem_Rotate_y", 0.0f);
    runtime.WriteFloat(document, "Attr_StripSystem_Rotate_z", 0.0f);
    runtime.WriteInt(document, "Attr_StripSystem_Color_Red", 255);
    runtime.WriteInt(document, "Attr_StripSystem_Color_Green", 255);
    runtime.WriteInt(document, "Attr_StripSystem_Color_Blue", 255);
    runtime.WriteInt(document, "Attr_StripSystem_Color_Alpha", 255);
    return document;
}

}  // namespace pal4
