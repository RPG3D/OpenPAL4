#include "pal4/revolute_default_document.h"

#include <array>

namespace pal4 {
namespace {

enum class Kind { String, Int, Float };

struct Default final {
    std::string_view key;
    Kind kind;
    std::string_view text{};
    std::int32_t integer = 0;
};

constexpr std::array<Default, 10u> kDefaults{{
    {"Attr_Class_Name", Kind::String, "RevoluteTask"},
    {"Attr_Task_Wait_Mode", Kind::Int, {}, 2},
    {"Attr_Task_Wait_Time", Kind::Float},
    {"Attr_Task_Expect_Time", Kind::Float},
    {"Attr_Task_Revolute_OriginPos_x", Kind::Float},
    {"Attr_Task_Revolute_OriginPos_y", Kind::Float},
    {"Attr_Task_Revolute_OriginPos_z", Kind::Float},
    {"Attr_Task_Revolute_Angle_x", Kind::Float},
    {"Attr_Task_Revolute_Angle_y", Kind::Float},
    {"Attr_Task_Revolute_Angle_z", Kind::Float},
}};

constexpr std::array<std::string_view, 3u> kVelocityKeys{
    "Attr_Task_Revolute_Center_Vel_x",
    "Attr_Task_Revolute_Center_Vel_y",
    "Attr_Task_Revolute_Center_Vel_z"};

void WriteDefault(const std::uint32_t document, const Default& value,
                  IRevoluteDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, value.key);
    if (existing != 0u) {
        switch (value.kind) {
        case Kind::String: runtime.SetString(existing, value.text); break;
        case Kind::Int: runtime.SetInt(existing, value.integer); break;
        case Kind::Float: runtime.SetFloat(existing, 0.0f); break;
        }
        return;
    }
    RevoluteAttributeTemporary temporary{};
    std::uint32_t attribute = 0u;
    switch (value.kind) {
    case Kind::String:
        attribute = runtime.InitializeStringAttribute(temporary, value.key,
                                                      value.text);
        break;
    case Kind::Int:
        attribute = runtime.InitializeIntAttribute(temporary, value.key,
                                                   value.integer);
        break;
    case Kind::Float:
        attribute = runtime.InitializeFloatAttribute(temporary, value.key,
                                                     0.0f);
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

std::uint32_t RevoluteDefaultDocument::Create(
    IRevoluteDefaultDocumentRuntime& runtime) {
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
    runtime.SetName(document, "RevoluteTask", 0u);
    for (const auto& value : kDefaults)
        WriteDefault(document, value, runtime);
    for (const auto key : kVelocityKeys)
        runtime.WriteFloat(document, key, 0.0f);
    return document;
}

}  // namespace pal4
