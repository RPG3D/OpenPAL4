#include "pal4/play_effect_default_document.h"

#include <array>

namespace pal4 {
namespace {

enum class Kind { String, Int, Float };

struct Default final {
    std::string_view key;
    Kind kind;
    std::int32_t integer = 0;
    bool tree_lookup = false;
};

constexpr std::array<Default, 5u> kDefaults{{
    {"Attr_Class_Name", Kind::String, 0, true},
    {"Attr_Task_Wait_Mode", Kind::Int, 1, true},
    {"Attr_Task_Wait_Time", Kind::Float, 0, true},
    {"Attr_Task_Expect_Time", Kind::Float},
    {"Attr_Task_PlayEffect_Loop", Kind::Int, 1, true},
}};

void WriteDefault(const std::uint32_t document, const Default& value,
                  IPlayEffectDefaultDocumentRuntime& runtime) {
    const auto existing = value.tree_lookup
                              ? runtime.FindTreeAttribute(document, value.key)
                              : runtime.FindAttribute(document, value.key);
    if (existing != 0u) {
        switch (value.kind) {
        case Kind::String:
            runtime.SetString(existing, "PlayEffectTask");
            break;
        case Kind::Int: runtime.SetInt(existing, value.integer); break;
        case Kind::Float: runtime.SetFloat(existing, 0.0f); break;
        }
        return;
    }

    PlayEffectAttributeTemporary temporary{};
    std::uint32_t attribute = 0u;
    switch (value.kind) {
    case Kind::String:
        attribute = runtime.InitializeStringAttribute(
            temporary, value.key, "PlayEffectTask");
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

std::uint32_t PlayEffectDefaultDocument::Create(
    IPlayEffectDefaultDocumentRuntime& runtime) {
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
    runtime.SetName(document, "PlayEffectTask", 0u);
    for (const auto& value : kDefaults)
        WriteDefault(document, value, runtime);
    return document;
}

}  // namespace pal4
