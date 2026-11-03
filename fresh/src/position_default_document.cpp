#include "pal4/position_default_document.h"

#include <array>

namespace pal4 {
namespace {
enum class Kind { String, Int, Float };
struct Default final {
    std::string_view key;
    Kind kind;
    std::int32_t integer = 0;
    bool tree = false;
};
constexpr std::array<Default, 8u> kDefaults{{
    {"Attr_Class_Name", Kind::String, 0, true},
    {"Attr_Task_Wait_Mode", Kind::Int, 2},
    {"Attr_Task_Wait_Time", Kind::Float},
    {"Attr_Task_Expect_Time", Kind::Float},
    {"Attr_Task_Position_Result", Kind::Int, 0, true},
    {"Attr_Task_Position_x", Kind::Float},
    {"Attr_Task_Position_y", Kind::Float},
    {"Attr_Task_Position_z", Kind::Float},
}};

void WriteDefault(std::uint32_t document, const Default& value,
                  IPositionDefaultDocumentRuntime& runtime) {
    const auto existing = value.tree
        ? runtime.FindTreeAttribute(document, value.key)
        : runtime.FindAttribute(document, value.key);
    if (existing != 0u) {
        switch (value.kind) {
        case Kind::String: runtime.SetString(existing, "PositionTask"); break;
        case Kind::Int: runtime.SetInt(existing, value.integer); break;
        case Kind::Float: runtime.SetFloat(existing, 0.0f); break;
        }
        return;
    }
    PositionAttributeTemporary temporary{};
    std::uint32_t attribute = 0u;
    switch (value.kind) {
    case Kind::String:
        attribute = runtime.InitializeStringAttribute(
            temporary, value.key, "PositionTask"); break;
    case Kind::Int:
        attribute = runtime.InitializeIntAttribute(
            temporary, value.key, value.integer); break;
    case Kind::Float:
        attribute = runtime.InitializeFloatAttribute(
            temporary, value.key, 0.0f); break;
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

std::uint32_t PositionDefaultDocument::Create(
    IPositionDefaultDocumentRuntime& runtime) {
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
    runtime.SetName(document, "PositionTask", 0u);
    for (const auto& value : kDefaults)
        WriteDefault(document, value, runtime);
    return document;
}

}  // namespace pal4
