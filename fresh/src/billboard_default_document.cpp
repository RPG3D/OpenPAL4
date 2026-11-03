#include "pal4/billboard_default_document.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::string_view kObjectName = "BillBordObject";
constexpr std::string_view kUnsetTexture = "\xCE\xB4\xC9\xE8\xD6\xC3";

struct FloatDefault {
    std::string_view key;
    float value;
};

constexpr std::array kFloatDefaults{
    FloatDefault{"Attr_BillBord_Width", 64.0f},
    FloatDefault{"Attr_BillBord_Height", 64.0f},
    FloatDefault{"Attr_BillBord_Scale_Width", 1.0f},
    FloatDefault{"Attr_BillBord_Scale_Height", 1.0f},
    FloatDefault{"Attr_BillBord_Rotate_x", 0.0f},
    FloatDefault{"Attr_BillBord_Position_x", 0.0f},
    FloatDefault{"Attr_BillBord_Position_y", 0.0f},
    FloatDefault{"Attr_BillBord_Position_z", 0.0f},
};

struct IntDefault {
    std::string_view key;
    std::int32_t value;
};

constexpr std::array kIntDefaults{
    IntDefault{"Attr_Auto_AlphaBlend", 1},
    IntDefault{"Attr_BillBord_AxisBillbord", 1},
    IntDefault{"Attr_BillBord_Color_Red", 255},
    IntDefault{"Attr_BillBord_Color_Green", 255},
    IntDefault{"Attr_BillBord_Color_Blue", 255},
    IntDefault{"Attr_BillBord_Color_Alpha", 255},
    IntDefault{"Attr_Render_Last", 1},
    IntDefault{"Attr_Center", 1},
};

void WriteString(std::uint32_t document, std::string_view key,
                 std::string_view value,
                 IBillboardDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetString(existing, value);
        return;
    }

    BillboardAttributeTemporary temporary{};
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

void WriteFloat(std::uint32_t document, const FloatDefault& entry,
                IBillboardDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, entry.key);
    if (existing != 0u) {
        runtime.SetFloat(existing, entry.value);
        return;
    }

    BillboardAttributeTemporary temporary{};
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

std::uint32_t BillboardDefaultDocument::Create(
    IBillboardDefaultDocumentRuntime& runtime) {
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
    WriteString(document, "Attr_Class_Name", kObjectName, runtime);
    WriteString(document, "Attr_BillBord_Texture", kUnsetTexture, runtime);
    for (const auto& entry : kFloatDefaults)
        WriteFloat(document, entry, runtime);
    for (const auto& entry : kIntDefaults)
        runtime.WriteInt(document, entry.key, entry.value);
    return document;
}

}  // namespace pal4
