#include "pal4/clump_default_document.h"

namespace pal4 {
namespace {

constexpr std::string_view kObjectName = "ClumpObject";
constexpr std::string_view kUnsetTexture = "\xCE\xB4\xC9\xE8\xD6\xC3";

struct FloatDefault final {
    std::string_view key;
    float value;
};

constexpr std::array kTransformDefaults{
    FloatDefault{"Attr_Clump_Scale_x", 1.0f},
    FloatDefault{"Attr_Clump_Scale_y", 1.0f},
    FloatDefault{"Attr_Clump_Scale_z", 1.0f},
    FloatDefault{"Attr_Clump_Rotate_x", 0.0f},
    FloatDefault{"Attr_Clump_Rotate_y", 0.0f},
    FloatDefault{"Attr_Clump_Rotate_z", 0.0f},
    FloatDefault{"Attr_Clump_Position_x", 0.0f},
    FloatDefault{"Attr_Clump_Position_y", 0.0f},
    FloatDefault{"Attr_Clump_Position_z", 0.0f},
};

void SetClassName(const std::uint32_t document,
                  IClumpDefaultDocumentRuntime& runtime) {
    constexpr std::string_view key = "Attr_Class_Name";
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetString(existing, kObjectName);
        return;
    }
    ClumpAttributeTemporary temporary{};
    const auto attribute = runtime.InitializeStringAttribute(
        temporary, key, kObjectName);
    try {
        runtime.AddAttribute(document, attribute);
    } catch (...) {
        runtime.DestroyAttribute(temporary);
        throw;
    }
    runtime.DestroyAttribute(temporary);
}

void SetTransform(const std::uint32_t document,
                  const FloatDefault& entry,
                  IClumpDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, entry.key);
    if (existing != 0u) {
        runtime.SetFloat(existing, entry.value);
        return;
    }
    ClumpAttributeTemporary temporary{};
    const auto attribute = runtime.InitializeFloatAttribute(
        temporary, entry.key, entry.value);
    try {
        runtime.AddAttribute(document, attribute);
    } catch (...) {
        runtime.DestroyAttribute(temporary);
        throw;
    }
    runtime.DestroyAttribute(temporary);
}

}  // namespace

std::uint32_t ClumpDefaultDocument::Create(
    IClumpDefaultDocumentRuntime& runtime) {
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
    SetClassName(document, runtime);
    for (const auto& entry : kTransformDefaults)
        SetTransform(document, entry, runtime);

    runtime.WriteInt(document, "Attr_Clump_Color_Red", 255);
    runtime.WriteInt(document, "Attr_Clump_Color_Green", 255);
    runtime.WriteInt(document, "Attr_Clump_Color_Blue", 255);
    runtime.WriteInt(document, "Attr_Clump_Color_Alpha", 255);
    runtime.WriteString(document, "Attr_Clump_FileName", "101\\101");
    runtime.WriteInt(document, "Attr_Clump_NorMap_Use", 0);
    runtime.WriteString(document, "Attr_Clump_NorMap_Texture",
                        kUnsetTexture);
    runtime.WriteFloat(document, "Attr_Clump_NorMap_Coeff", 1.0f);
    runtime.WriteInt(document, "Attr_Clump_RefMap_Use", 0);
    runtime.WriteString(document, "Attr_Clump_RefMap_Texture",
                        kUnsetTexture);
    runtime.WriteFloat(document, "ATTR_Clump_RefMap_Coeff", 0.8f);
    runtime.WriteInt(document, "Attr_Clump_DualMap_Use", 0);
    runtime.WriteString(document, "Attr_Clump_DualMap_Texture",
                        kUnsetTexture);
    return document;
}

}  // namespace pal4
