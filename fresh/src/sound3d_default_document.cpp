#include "pal4/sound3d_default_document.h"

namespace pal4 {
namespace {

enum class Kind { String, Int, Float };
struct Default final {
    std::string_view key;
    Kind kind;
    std::string_view text;
};

constexpr std::array<Default, 6> kDefaults{{
    {"Attr_Class_Name", Kind::String, "Sound3DObject"},
    {"Attr_Sound3D_SoundPos_x", Kind::Float, {}},
    {"Attr_Sound3D_SoundPos_y", Kind::Float, {}},
    {"Attr_Sound3D_SoundPos_z", Kind::Float, {}},
    {"Attr_Sound3D_SoundFile", Kind::String, "type.wav"},
    {"Attr_Sound3D_2D", Kind::Int, {}},
}};

void WriteDefault(std::uint32_t document, const Default& value,
                  ISound3DDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, value.key);
    if (existing != 0u) {
        switch (value.kind) {
        case Kind::String: runtime.SetString(existing, value.text); break;
        case Kind::Int: runtime.SetInt(existing, 0); break;
        case Kind::Float: runtime.SetFloat(existing, 0.0f); break;
        }
        return;
    }

    Sound3DAttributeTemporary temporary{};
    std::uint32_t attribute = 0u;
    switch (value.kind) {
    case Kind::String:
        attribute = runtime.InitializeStringAttribute(
            temporary, value.key, value.text);
        break;
    case Kind::Int:
        attribute = runtime.InitializeIntAttribute(temporary, value.key, 0);
        break;
    case Kind::Float:
        attribute = runtime.InitializeFloatAttribute(temporary, value.key, 0.0f);
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

std::uint32_t Sound3DObjectDefaultDocument::Create(
    ISound3DDefaultDocumentRuntime& runtime) {
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
    runtime.SetName(document, "Sound3DObject", 0u);
    for (const auto& value : kDefaults)
        WriteDefault(document, value, runtime);
    return document;
}

}  // namespace pal4
