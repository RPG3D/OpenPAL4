#pragma once

#include "pal4/strip_object.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

struct StripVisualParameters final {
    float width = 0.0f;
    float height = 0.0f;
    bool flip_uv = false;
    bool alpha_blend = true;
    bool cylinder = true;
    std::int32_t cylinder_borders = 10;
    float cylinder_top_radius = 5.0f;
    float cylinder_bottom_radius = 10.0f;
    std::int32_t cylinder_loops = 1;
    bool straight = false;
    std::int32_t straight_faces = 1;
    bool sequence = false;
    float sequence_width = 16.0f;
    float sequence_height = 16.0f;
    float sequence_fps = 30.0f;
    bool center = true;
    bool render_last = true;
    bool render_state = false;
    std::int32_t cull_mode = 0;
};

class IStripConfigurationRuntime {
public:
    virtual ~IStripConfigurationRuntime() = default;
    [[nodiscard]] virtual std::string_view ReadTextureName(
        std::uint32_t document, std::string_view key,
        std::uint32_t default_address) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t document, std::string_view key,
        std::int32_t default_value) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t document, std::string_view key,
        float default_value) = 0;
    [[nodiscard]] virtual std::uint32_t FindTexture(std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t LoadTexture(std::string_view path) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(float value) = 0;
    virtual void TextureSize(std::uint32_t texture, std::int32_t& width,
                             std::int32_t& height) = 0;
    virtual void CreateCylinder(StripObjectState& state,
                                std::uint32_t texture,
                                const StripVisualParameters& parameters) = 0;
    virtual void CreateStraight(StripObjectState& state,
                                std::uint32_t texture,
                                const StripVisualParameters& parameters) = 0;
    [[nodiscard]] virtual std::uint32_t CopyDocument(
        std::uint32_t document) = 0;
    virtual void ReleaseTexture(std::uint32_t texture) = 0;
    virtual void ReleaseTexturePath(std::string_view path) = 0;
};

class StripConfiguration final {
public:
    // IDA 0x618af0.
    [[nodiscard]] static std::uint8_t Load(
        StripObjectState& state, std::uint32_t document,
        IStripConfigurationRuntime& runtime);
};

}  // namespace pal4
