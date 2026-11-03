#include "pal4/sequence_anim_default_document.h"

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
constexpr Default S(std::string_view key, std::string_view value) {
    return {key, Kind::String, 0u, value};
}
constexpr Default I(std::string_view key, std::int32_t value) {
    return {key, Kind::Int, static_cast<std::uint32_t>(value), {}};
}
constexpr Default F(std::string_view key, float value) {
    return {key, Kind::Float, std::bit_cast<std::uint32_t>(value), {}};
}

constexpr std::array<Default, 18> kDefaults{{
    S("Attr_Class_Name", "SequnceAnimObject"),
    S("Attr_SequenceAnim_Texture", "SequenceAnim\\baozha6 A.png"),
    I("Attr_Auto_AlphaBlend", 1),
    I("Attr_SequenceAnim_FPS", 60),
    I("Attr_SequenceAnim_AxisBillbord", 0),
    F("Attr_SequenceAnim_SubTexWidth", 64.0f),
    F("Attr_SequenceAnim_SubTexHeight", 64.0f),
    F("Attr_SequenceAnim_Scale_SubTexWidth", 1.0f),
    F("Attr_SequenceAnim_Scale_SubTexHeight", 1.0f),
    F("Attr_SequenceAnim_Rotate_x", 0.0f),
    F("Attr_SequenceAnim_Position_X", 0.0f),
    F("Attr_SequenceAnim_Position_Y", 0.0f),
    F("Attr_SequenceAnim_Position_Z", 0.0f),
    I("Attr_SequenceAnim_Color_Red", 255),
    I("Attr_SequenceAnim_Color_Green", 255),
    I("Attr_SequenceAnim_Color_Blue", 255),
    I("Attr_SequenceAnim_Color_Alpha", 255),
    I("Attr_Render_Last", 1),
}};

void SetAttribute(std::uint32_t document, const Default& value,
                  ISequenceAnimDefaultDocumentRuntime& runtime) {
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

    SequenceAnimAttributeTemporary temporary{};
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

std::uint32_t SequenceAnimObjectDefaultDocument::Create(
    ISequenceAnimDefaultDocumentRuntime& runtime) {
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
    runtime.SetName(document, "SequnceAnimObject", 0u);
    for (const auto& value : kDefaults)
        SetAttribute(document, value, runtime);
    return document;
}

}  // namespace pal4
