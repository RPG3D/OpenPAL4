#include "pal4/strip_default_document.h"

namespace pal4 {
namespace {

constexpr std::string_view kObjectName = "StripObject";
constexpr std::string_view kUnsetTexture = "\xCE\xB4\xC9\xE8\xD6\xC3";

void SetString(const std::uint32_t document, const std::string_view key,
               const std::string_view value,
               IStripDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetString(existing, value);
        return;
    }
    StripAttributeTemporary temporary{};
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
            IStripDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetInt(existing, value);
        return;
    }
    StripAttributeTemporary temporary{};
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
              const float value, IStripDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetFloat(existing, value);
        return;
    }
    StripAttributeTemporary temporary{};
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

std::uint32_t StripDefaultDocument::Create(
    IStripDefaultDocumentRuntime& runtime) {
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
    SetString(document, "Attr_Strip_Texture", kUnsetTexture, runtime);
    SetInt(document, "Attr_Auto_AlphaBlend", 1, runtime);
    SetInt(document, "Attr_Strip_FlipUV", 0, runtime);
    SetFloat(document, "Attr_Strip_Wdith", 20.0f, runtime);
    SetFloat(document, "Attr_Strip_Height", 10.0f, runtime);
    SetInt(document, "Attr_Strip_Vertex_Col_Red", 255, runtime);
    SetInt(document, "Attr_Strip_Vertex_Col_Green", 255, runtime);
    SetInt(document, "Attr_Strip_Vertex_Col_Blue", 255, runtime);
    SetInt(document, "Attr_Strip_Vertex_Col_Alpha", 255, runtime);
    SetFloat(document, "Attr_Strip_Position_x", 0.0f, runtime);
    SetFloat(document, "Attr_Strip_Position_y", 0.0f, runtime);
    SetFloat(document, "Attr_Strip_Position_z", 0.0f, runtime);
    runtime.WriteFloat(document, "Attr_Strip_Scale_x", 1.0f);
    runtime.WriteFloat(document, "Attr_Strip_Scale_y", 1.0f);
    runtime.WriteFloat(document, "Attr_Strip_Scale_z", 1.0f);
    runtime.WriteFloat(document, "Attr_Strip_Rotate_x", 0.0f);
    runtime.WriteFloat(document, "Attr_Strip_Rotate_y", 0.0f);
    runtime.WriteFloat(document, "Attr_Strip_Rotate_z", 0.0f);
    runtime.WriteInt(document, "Attr_Strip_Cylinder_Creator", 1);
    runtime.WriteInt(document, "Attr_Strip_Cylinder_Border_Numb", 10);
    runtime.WriteFloat(document, "Attr_Strip_Cylinder_TRadius", 10.0f);
    runtime.WriteFloat(document, "Attr_Strip_Cylinder_BRadius", 10.0f);
    runtime.WriteInt(document, "Attr_Strip_Cylinder_Loop_Numb", 2);
    runtime.WriteInt(document, "Attr_Strip_Straight_Creator", 0);
    runtime.WriteInt(document, "Attr_Strip_Straight_Face_Numb", 1);
    runtime.WriteInt(document, "Attr_Strip_Straight_Sequence", 0);
    runtime.WriteFloat(document, "Attr_Strip_Straight_Seq_Width", 16.0f);
    runtime.WriteFloat(document, "Attr_Strip_Straight_Seq_Height", 16.0f);
    runtime.WriteFloat(document, "Attr_Strip_Straight_Seq_Fps", 30.0f);
    return document;
}

}  // namespace pal4
