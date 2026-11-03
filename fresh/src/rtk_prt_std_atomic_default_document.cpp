#include "pal4/rtk_prt_std_atomic_default_document.h"

#include <bit>

namespace pal4 {
namespace {

enum class Kind { String, Int, Float };
struct Default final {
    std::string_view key;
    Kind kind;
    std::uint32_t bits;
    std::string_view text;
};

constexpr Default S(std::string_view key, std::string_view text) {
    return {key, Kind::String, 0u, text};
}
constexpr Default I(std::string_view key, std::uint32_t value) {
    return {key, Kind::Int, value, {}};
}
constexpr Default F(std::string_view key, float value) {
    return {key, Kind::Float, std::bit_cast<std::uint32_t>(value), {}};
}

// Execution order from IDA 0x6117a0. The first 30 entries are inlined;
// the remaining 45 call the native typed attribute writers.
constexpr std::array<Default, 75> kDefaults{{
    S("Attr_Class_Name", "RtkPrtStdAtomicObject"),
    I("Attr_Prt_MaxPrt", 100),
    I("Attr_Prt_PrtEmit", 20),
    I("Attr_Prt_PrtEmit_Bias", 5),
    F("Attr_Prt_EmitGap", 0.0f),
    F("Attr_Prt_EmitGap_Bias", 0.0f),
    I("Attr_Prt_Alpha_AutoGradual", 1),
    F("Attr_PrtSize_x", 1.0f),
    F("Attr_PrtSize_y", 1.0f),
    F("Attr_Prt_2DRotate", 0.0f),
    F("Attr_Prt_PrtLife", 1.0f),
    F("Attr_Prt_PrtLife_Bias", 1.0f),
    F("Attr_Prt_InitVel", 1.0f),
    F("Attr_Prt_initVel_Bias", 0.0f),
    F("Attr_Prt_InitDir_x", 0.0f),
    F("Attr_Prt_InitDir_y", 1.0f),
    F("Attr_Prt_InitDir_z", 0.0f),
    F("Attr_Prt_InitDir_Bias_x", 0.0f),
    F("Attr_Prt_InitDir_Bias_y", 0.0f),
    F("Attr_Prt_InitDir_Bias_z", 0.0f),
    I("Attr_Prt_Color_Red", 255),
    I("Attr_Prt_Color_Green", 255),
    I("Attr_Prt_Color_Blue", 255),
    I("Attr_Prt_Color_Alpha", 255),
    F("Attr_Prt_Gravitate", 0.0f),
    F("Attr_Prt_Gravitate_Pos_x", 0.0f),
    F("Attr_Prt_Gravitate_Pos_y", 0.0f),
    F("Attr_Prt_Gravitate_Pos_z", 0.0f),
    F("Attr_Prt_Force_x", 0.0f),
    I("Attr_Prt_ColorGradual", 1),
    I("Attr_Prt_StartCol_Red", 255),
    I("Attr_Prt_StartCol_Green", 255),
    I("Attr_Prt_StartCol_Blue", 255),
    I("Attr_Prt_StartCol_Alpha", 255),
    I("Attr_Prt_StartColBias_Red", 0),
    I("Attr_Prt_StartColBias_Green", 0),
    I("Attr_Prt_StartColBias_Blue", 0),
    I("Attr_Prt_StartColBias_Alpha", 0),
    I("Attr_Prt_EndCol_Red", 0),
    I("Attr_Prt_EndCol_Green", 0),
    I("Attr_Prt_EndCol_Blue", 0),
    I("Attr_Prt_EndCol_Alpha", 0),
    I("Attr_Prt_EndColBias_Red", 0),
    I("Attr_Prt_EndColBias_Green", 0),
    I("Attr_Prt_EndColBias_Blue", 0),
    I("Attr_Prt_EndColBias_Alpha", 0),
    I("Attr_Prt_SizeGradual", 1),
    F("Attr_Prt_StartSize_x", 0.0f),
    F("Attr_Prt_StartSize_y", 0.0f),
    F("Attr_Prt_StartSizeBias_x", 0.0f),
    F("Attr_Prt_StartSizeBias_y", 0.0f),
    F("Attr_Prt_EndSize_x", 16.0f),
    F("Attr_Prt_EndSize_y", 16.0f),
    F("Attr_Prt_EndSizeBias_x", 0.0f),
    F("Attr_Prt_EndSizeBias_y", 0.0f),
    I("Attr_Prt_2DRotateGradual", 0),
    F("Attr_Prt_Start2DRotate", 10.0f),
    F("Attr_Prt_Start2DRotateBias", 2.0f),
    F("Attr_Prt_End2DRotate", 0.0f),
    F("Attr_Prt_End2DRotateBias", 0.0f),
    F("Attr_Prt_Atomic_Rotate_x", 0.0f),
    F("Attr_Prt_Atomic_Rotate_y", 0.0f),
    F("Attr_Prt_Atomic_Rotate_z", 0.0f),
    F("Attr_Prt_Atomic_Scale_x", 1.0f),
    F("Attr_Prt_Atomic_Scale_y", 1.0f),
    F("Attr_Prt_Atomic_Scale_z", 1.0f),
    F("Attr_Prt_Atomic_Position_x", 0.0f),
    F("Attr_Prt_Atomic_Position_y", 0.0f),
    F("Attr_Prt_Atomic_Position_z", 0.0f),
    S("Attr_Prt_EmitterType", "\xB3\xA4\xB7\xBD\xCC\xE5"),
    F("Attr_Prt_Size_x", 10.0f),
    F("Attr_Prt_Size_y", 10.0f),
    F("Attr_Prt_Size_z", 10.0f),
    S("Attr_Prt_Texture", "gloss_64"),
    F("Attr_PreRender_Time", 0.0f),
}};

void SetAttribute(std::uint32_t document, const Default& value,
                  IRtkPrtStdAtomicDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, value.key);
    const auto float_value = std::bit_cast<float>(value.bits);
    if (existing != 0u) {
        switch (value.kind) {
        case Kind::String: runtime.SetString(existing, value.text); break;
        case Kind::Int: runtime.SetInt(existing, static_cast<std::int32_t>(value.bits)); break;
        case Kind::Float: runtime.SetFloat(existing, float_value); break;
        }
        return;
    }

    RtkPrtStdAtomicAttributeTemporary temporary{};
    std::uint32_t attribute = 0u;
    switch (value.kind) {
    case Kind::String:
        attribute = runtime.InitializeStringAttribute(temporary, value.key, value.text);
        break;
    case Kind::Int:
        attribute = runtime.InitializeIntAttribute(
            temporary, value.key, static_cast<std::int32_t>(value.bits));
        break;
    case Kind::Float:
        attribute = runtime.InitializeFloatAttribute(temporary, value.key, float_value);
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

std::uint32_t RtkPrtStdAtomicObjectDefaultDocument::Create(
    IRtkPrtStdAtomicDefaultDocumentRuntime& runtime) {
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
    runtime.SetName(document, "RtkPrtStdAtomicObject", 0u);
    for (const auto& value : kDefaults)
        SetAttribute(document, value, runtime);
    return document;
}

}  // namespace pal4
