#pragma once

#include "pal4/sequence_anim_object.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ISequenceAnimConfigurationRuntime {
public:
    virtual ~ISequenceAnimConfigurationRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t document, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t document, std::string_view key, float fallback) = 0;
    [[nodiscard]] virtual std::string ReadString(
        std::uint32_t document, std::string_view key,
        std::string_view fallback) = 0;
    [[nodiscard]] virtual std::uint32_t FindTexture(std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t LoadTexture(std::string_view path) = 0;
    [[nodiscard]] virtual std::int32_t TextureWidth(std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::int32_t TextureHeight(std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::int32_t Ftol(float value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateResource(std::uint32_t bytes) = 0;
    virtual void WriteResourceWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteResourceByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual bool InitializeResource(
        std::uint32_t resource, std::uint32_t texture,
        float width, float height, bool auto_blend,
        bool axis_billboard, bool center) = 0;
    virtual void InitializeTransform(std::uint32_t resource) = 0;
    [[nodiscard]] virtual std::uint32_t Geometry(std::uint32_t resource) = 0;
    virtual void SetColor(std::uint32_t geometry, std::uint32_t rgba) = 0;
    virtual void UpdateGeometry(std::uint32_t geometry,
                                const std::array<std::uint32_t, 4>& corners) = 0;
    virtual void DeinitializeResource(std::uint32_t resource) = 0;
    virtual void FreeResource(std::uint32_t resource) = 0;
};

class SequenceAnimConfiguration final {
public:
    // IDA 0x615a80.
    [[nodiscard]] static bool Load(SequenceAnimObjectState& state,
                                   std::uint32_t document,
                                   ISequenceAnimConfigurationRuntime& runtime);
};

}  // namespace pal4
