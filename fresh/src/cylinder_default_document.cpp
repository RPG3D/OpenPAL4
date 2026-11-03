#include "pal4/cylinder_default_document.h"

namespace pal4 {
namespace {

constexpr std::string_view kObjectName = "CylinderObject";
constexpr std::string_view kUnsetTexture = "\xCE\xB4\xC9\xE8\xD6\xC3";

void SetStringAttribute(const std::uint32_t document,
                        const std::string_view key,
                        const std::string_view value,
                        ICylinderDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetString(existing, value);
        return;
    }
    CylinderAttributeTemporary temporary{};
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
                     ICylinderDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetInt(existing, value);
        return;
    }
    CylinderAttributeTemporary temporary{};
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
                       ICylinderDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetFloat(existing, value);
        return;
    }
    CylinderAttributeTemporary temporary{};
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

std::uint32_t CylinderDefaultDocument::Create(
    ICylinderDefaultDocumentRuntime& runtime) {
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
    SetStringAttribute(document, "Attr_Cylinder_Texture", kUnsetTexture,
                       runtime);
    SetIntAttribute(document, "Attr_Auto_AlphaBlend", 1, runtime);
    SetIntAttribute(document, "Attr_Cylinder_FlipUV", 1, runtime);
    SetIntAttribute(document, "Attr_Cylinder_Center", 1, runtime);
    SetIntAttribute(document, "Attr_Cylinder_BorderNumb", 5, runtime);
    SetFloatAttribute(document, "Attr_Cylinder_Height", 5.0f, runtime);
    SetFloatAttribute(document, "Attr_Cylinder_Bottom_Raidus", 6.0f, runtime);
    SetFloatAttribute(document, "Attr_Cylinder_Top_Raidus", 3.0f, runtime);
    SetIntAttribute(document, "Attr_Cylinder_Angel", 360, runtime);
    SetIntAttribute(document, "Attr_Cylinder_Asterialize", 0, runtime);

    runtime.WriteFloat(document, "Attr_Cylinder_Shrink_Size", 2.0f);
    runtime.WriteInt(document, "Attr_Cylinder_Vertex_Col_Red", 255);
    runtime.WriteInt(document, "Attr_Cylinder_Vertex_Col_Green", 255);
    runtime.WriteInt(document, "Attr_Cylinder_Vertex_Col_Blue", 255);
    runtime.WriteInt(document, "Attr_Cylinder_Vertex_Col_Alpha", 255);
    runtime.WriteFloat(document, "Attr_Cylinder_Position_x", 0.0f);
    runtime.WriteFloat(document, "Attr_Cylinder_Position_y", 0.0f);
    runtime.WriteFloat(document, "Attr_Cylinder_Position_z", 0.0f);
    runtime.WriteFloat(document, "Attr_Cylinder_Scale_x", 1.0f);
    runtime.WriteFloat(document, "Attr_Cylinder_Scale_y", 1.0f);
    runtime.WriteFloat(document, "Attr_Cylinder_Scale_z", 1.0f);
    runtime.WriteFloat(document, "Attr_Cylinder_Rotate_x", 0.0f);
    runtime.WriteFloat(document, "Attr_Cylinder_Rotate_y", 0.0f);
    runtime.WriteFloat(document, "Attr_Cylinder_Rotate_z", 0.0f);
    return document;
}

}  // namespace pal4
