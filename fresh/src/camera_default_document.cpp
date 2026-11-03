#include "pal4/camera_default_document.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::string_view kObjectName = "CameraObject";

struct FloatDefault final {
    std::string_view key;
    float value;
};

constexpr std::array kFloatDefaults{
    FloatDefault{"Attr_Camera_Position_X", 0.0f},
    FloatDefault{"Attr_Camera_Position_Y", 100.0f},
    FloatDefault{"Attr_Camera_Position_Z", 100.0f},
    FloatDefault{"Attr_Camera_LookAt_x", 0.0f},
    FloatDefault{"Attr_Camera_LookAt_y", 0.0f},
    FloatDefault{"Attr_Camera_LookAt_z", 0.0f},
    FloatDefault{"Attr_Camera_Pitch", 45.0f},
    FloatDefault{"Attr_Camera_Yaw", 180.0f},
};

void WriteString(const std::uint32_t document, std::string_view key,
                 std::string_view value,
                 ICameraDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetString(existing, value);
        return;
    }
    CameraAttributeTemporary temporary{};
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

void WriteInt(const std::uint32_t document, std::string_view key,
              const std::int32_t value,
              ICameraDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, key);
    if (existing != 0u) {
        runtime.SetInt(existing, value);
        return;
    }
    CameraAttributeTemporary temporary{};
    const auto attribute = runtime.InitializeIntAttribute(
        temporary, key, value);
    try {
        runtime.AddAttribute(document, attribute);
    } catch (...) {
        runtime.DestroyAttribute(temporary);
        throw;
    }
    runtime.DestroyAttribute(temporary);
}

void WriteFloat(const std::uint32_t document,
                const FloatDefault& entry,
                ICameraDefaultDocumentRuntime& runtime) {
    const auto existing = runtime.FindAttribute(document, entry.key);
    if (existing != 0u) {
        runtime.SetFloat(existing, entry.value);
        return;
    }
    CameraAttributeTemporary temporary{};
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

std::uint32_t CameraDefaultDocument::Create(
    ICameraDefaultDocumentRuntime& runtime) {
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
    WriteInt(document, "Attr_Camera_Lock_LookAt", 1, runtime);
    for (const auto& entry : kFloatDefaults)
        WriteFloat(document, entry, runtime);
    runtime.WriteFloat(document, "Attr_Camera_Roll", 0.0f);
    return document;
}

}  // namespace pal4
