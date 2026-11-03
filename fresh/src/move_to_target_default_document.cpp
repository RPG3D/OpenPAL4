#include "pal4/move_to_target_default_document.h"

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
    {"Attr_Class_Name", Kind::String, "MoveToTargetTask"},
    {"Attr_Task_Wait_Mode", Kind::Int, {}, 2},
    {"Attr_Task_Wait_Time", Kind::Float},
    {"Attr_Task_Expect_Time", Kind::Float},
    {"Attr_Task_Target_Name", Kind::String, ""},
    {"Attr_Task_Target_Position_x", Kind::Float},
    {"Attr_Task_Target_Position_y", Kind::Float},
    {"Attr_Task_Target_Position_z", Kind::Float},
    {"Attr_Task_Target_Rotate_x", Kind::Float},
    {"Attr_Task_Target_Rotate_y", Kind::Float},
}};

void WriteDefault(const std::uint32_t document, const Default& value,
                  IMoveToTargetDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, value.key);
    if (existing != 0u) {
        switch (value.kind) {
        case Kind::String: runtime.SetString(existing, value.text); break;
        case Kind::Int: runtime.SetInt(existing, value.integer); break;
        case Kind::Float: runtime.SetFloat(existing, 0.0f); break;
        }
        return;
    }

    MoveToTargetAttributeTemporary temporary{};
    std::uint32_t attribute = 0u;
    switch (value.kind) {
    case Kind::String:
        attribute = runtime.InitializeStringAttribute(
            temporary, value.key, value.text);
        break;
    case Kind::Int:
        attribute = runtime.InitializeIntAttribute(
            temporary, value.key, value.integer);
        break;
    case Kind::Float:
        attribute = runtime.InitializeFloatAttribute(
            temporary, value.key, 0.0f);
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

std::uint32_t MoveToTargetDefaultDocument::Create(
    IMoveToTargetDefaultDocumentRuntime& runtime) {
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
    runtime.SetName(document, "MoveToTargetTask", 0u);
    for (const auto& value : kDefaults)
        WriteDefault(document, value, runtime);
    runtime.WriteFloat(document, "Attr_Task_Target_Rotate_z", 0.0f);
    return document;
}

}  // namespace pal4
