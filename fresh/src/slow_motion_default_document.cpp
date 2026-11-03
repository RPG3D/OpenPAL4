#include "pal4/slow_motion_default_document.h"

#include <array>

namespace pal4 {
namespace {

enum class Kind { String, Int, Float };
struct Default final {
    std::string_view key;
    Kind kind;
    float value;
    bool tree_lookup;
};
constexpr std::array<Default, 5u> kDefaults{{
    {"Attr_Class_Name", Kind::String, 0.0f, true},
    {"Attr_Task_Wait_Mode", Kind::Int, 2.0f, true},
    {"Attr_Task_Wait_Time", Kind::Float, 0.0f, true},
    {"Attr_Task_Expect_Time", Kind::Float, 0.0f, false},
    {"Attr_Task_SlowMotion_ScaleTime", Kind::Float, 0.5f, true},
}};

void WriteDefault(std::uint32_t document, const Default& value,
                  ISlowMotionDefaultDocumentRuntime& runtime) {
    const auto existing = value.tree_lookup
        ? runtime.FindTreeAttribute(document, value.key)
        : runtime.FindAttribute(document, value.key);
    if (existing != 0u) {
        switch (value.kind) {
        case Kind::String: runtime.SetString(existing, "SlowMotionTask"); break;
        case Kind::Int: runtime.SetInt(existing, 2); break;
        case Kind::Float: runtime.SetFloat(existing, value.value); break;
        }
        return;
    }

    SlowMotionAttributeTemporary temporary{};
    std::uint32_t attribute = 0u;
    switch (value.kind) {
    case Kind::String:
        attribute = runtime.InitializeStringAttribute(
            temporary, value.key, "SlowMotionTask");
        break;
    case Kind::Int:
        attribute = runtime.InitializeIntAttribute(temporary, value.key, 2);
        break;
    case Kind::Float:
        attribute = runtime.InitializeFloatAttribute(
            temporary, value.key, value.value);
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

std::uint32_t SlowMotionDefaultDocument::Create(
    ISlowMotionDefaultDocumentRuntime& runtime) {
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
    runtime.SetName(document, "SlowMotionTask", 0u);
    for (const auto& value : kDefaults)
        WriteDefault(document, value, runtime);
    return document;
}

}  // namespace pal4
