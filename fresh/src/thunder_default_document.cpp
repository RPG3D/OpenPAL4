#include "pal4/thunder_default_document.h"

namespace pal4 {
namespace {

constexpr std::string_view kObjectName = "ThunderObject";

void SetString(const std::uint32_t document, const std::string_view key,
               const std::string_view value,
               IThunderDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetString(existing, value);
        return;
    }
    ThunderAttributeTemporary temporary{};
    const auto attribute = runtime.InitializeStringAttribute(temporary, key, value);
    try {
        runtime.AddAttribute(document, attribute);
    } catch (...) {
        runtime.DestroyAttribute(temporary);
        throw;
    }
    runtime.DestroyAttribute(temporary);
}

void SetInt(const std::uint32_t document, const std::string_view key,
            const std::int32_t value,
            IThunderDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetInt(existing, value);
        return;
    }
    ThunderAttributeTemporary temporary{};
    const auto attribute = runtime.InitializeIntAttribute(temporary, key, value);
    try {
        runtime.AddAttribute(document, attribute);
    } catch (...) {
        runtime.DestroyAttribute(temporary);
        throw;
    }
    runtime.DestroyAttribute(temporary);
}

void SetFloat(const std::uint32_t document, const std::string_view key,
              const float value,
              IThunderDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetFloat(existing, value);
        return;
    }
    ThunderAttributeTemporary temporary{};
    const auto attribute = runtime.InitializeFloatAttribute(temporary, key, value);
    try {
        runtime.AddAttribute(document, attribute);
    } catch (...) {
        runtime.DestroyAttribute(temporary);
        throw;
    }
    runtime.DestroyAttribute(temporary);
}

}  // namespace

std::uint32_t ThunderDefaultDocument::Create(
    IThunderDefaultDocumentRuntime& runtime) {
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
    SetString(document, "Attr_Class_Name", kObjectName, runtime);
    SetString(document, "Attr_Thunder_Texture", "Bolt.png", runtime);
    SetInt(document, "Attr_Thunder_AlphaBlend", 1, runtime);
    SetInt(document, "Attr_Thunder_Local_Model", 1, runtime);
    SetInt(document, "Attr_Thunder_Lock_Target", 1, runtime);
    SetInt(document, "Attr_Thunder_Size_Gradual", 0, runtime);
    SetString(document, "Attr_Thunder_Target_Name", "", runtime);
    SetFloat(document, "Attr_Thunder_Fps", 60.0f, runtime);
    SetFloat(document, "Attr_Thunder_MinRange", 0.02f, runtime);
    SetFloat(document, "Attr_Thunder_MaxRange", 0.02f, runtime);
    SetFloat(document, "Attr_Thunder_Target_x", 0.0f, runtime);
    SetFloat(document, "Attr_Thunder_Target_y", 10.0f, runtime);
    SetFloat(document, "Attr_Thunder_Target_z", 0.0f, runtime);
    runtime.WriteFloat(document, "Attr_Thunder_Section_Min_Width", 0.01f);
    runtime.WriteFloat(document, "Attr_Thunder_Section_Max_Width", 0.02f);
    runtime.WriteFloat(document, "Attr_Thunder_Section_Min_Height", 0.1f);
    runtime.WriteFloat(document, "Attr_Thunder_Section_Max_Height", 0.5f);
    runtime.WriteFloat(document, "Attr_Thunder_Scale_x", 1.0f);
    runtime.WriteFloat(document, "Attr_Thunder_Scale_y", 1.0f);
    runtime.WriteFloat(document, "Attr_Thunder_Scale_z", 1.0f);
    runtime.WriteFloat(document, "Attr_Thunder_Rotate_x", 0.0f);
    runtime.WriteFloat(document, "Attr_Thunder_Rotate_y", 0.0f);
    runtime.WriteFloat(document, "Attr_Thunder_Rotate_z", 0.0f);
    runtime.WriteFloat(document, "Attr_Thunder_Position_x", 0.0f);
    runtime.WriteFloat(document, "Attr_Thunder_Position_y", 0.0f);
    runtime.WriteFloat(document, "Attr_Thunder_Position_z", 0.0f);
    runtime.WriteInt(document, "Attr_Thunder_Color_Red", 255);
    runtime.WriteInt(document, "Attr_Thunder_Color_Green", 255);
    runtime.WriteInt(document, "Attr_Thunder_Color_Blue", 255);
    runtime.WriteInt(document, "Attr_Thunder_Color_Alpha", 180);
    return document;
}

}  // namespace pal4
