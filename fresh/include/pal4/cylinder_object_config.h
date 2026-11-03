#pragma once

#include "pal4/cylinder_object.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct CylinderVisualParameters final {
    std::int32_t border_count = 5;
    float height = 5.0f;
    float bottom_radius = 6.0f;
    float top_radius = 3.0f;
    bool rasterize = true;
    float shrink_size = 2.0f;
    std::int32_t angle = 360;
    bool flip_uv = false;
    bool center = true;
    bool auto_alpha_blend = true;
};

class ICylinderObjectConfigRuntime {
public:
    virtual ~ICylinderObjectConfigRuntime() = default;
    [[nodiscard]] virtual std::string_view ReadTextureName(
        std::uint32_t document, std::string_view key,
        std::uint32_t default_address) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t document, std::string_view key,
        std::int32_t default_value) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t document, std::string_view key,
        float default_value) = 0;
    [[nodiscard]] virtual std::uint32_t FindTexture(
        std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t LoadTexture(
        std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateModel(
        std::uint32_t bytes) = 0;
    virtual void InitializeModel(std::uint32_t model) = 0;
    [[nodiscard]] virtual bool CreateVisualModel(
        std::uint32_t model, std::uint32_t texture,
        const CylinderVisualParameters& parameters) = 0;
    virtual void SetPosition(std::uint32_t model,
                             const std::array<float, 3>& position,
                             std::uint32_t mode) = 0;
    virtual void SetRotation(std::uint32_t model,
                             const std::array<float, 3>& rotation,
                             std::uint32_t mode) = 0;
    virtual void SetScale(std::uint32_t model,
                          const std::array<float, 3>& scale,
                          std::uint32_t mode) = 0;
    virtual void SetColor(std::uint32_t model, std::uint32_t rgba) = 0;
    [[nodiscard]] virtual std::uint32_t CopyDocument(
        std::uint32_t document) = 0;
    virtual void DestroyModel(std::uint32_t model) = 0;
    virtual void FreeModel(std::uint32_t model) = 0;
    virtual void ReleaseTexturePath(std::string_view path) = 0;
};

class CylinderObjectConfigRuntimeStub final
    : public ICylinderObjectConfigRuntime {
public:
    [[nodiscard]] std::string_view ReadTextureName(
        std::uint32_t, std::string_view, std::uint32_t) override { return {}; }
    [[nodiscard]] std::int32_t ReadInt(
        std::uint32_t, std::string_view,
        std::int32_t value) override { return value; }
    [[nodiscard]] float ReadFloat(
        std::uint32_t, std::string_view,
        float value) override { return value; }
    [[nodiscard]] std::uint32_t FindTexture(std::string_view) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t LoadTexture(std::string_view) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t AllocateModel(std::uint32_t) override {
        return 0u;
    }
    void InitializeModel(std::uint32_t) override {}
    [[nodiscard]] bool CreateVisualModel(
        std::uint32_t, std::uint32_t,
        const CylinderVisualParameters&) override { return false; }
    void SetPosition(std::uint32_t, const std::array<float, 3>&,
                     std::uint32_t) override {}
    void SetRotation(std::uint32_t, const std::array<float, 3>&,
                     std::uint32_t) override {}
    void SetScale(std::uint32_t, const std::array<float, 3>&,
                  std::uint32_t) override {}
    void SetColor(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t CopyDocument(std::uint32_t) override {
        return 0u;
    }
    void DestroyModel(std::uint32_t) override {}
    void FreeModel(std::uint32_t) override {}
    void ReleaseTexturePath(std::string_view) override {}
};

class CylinderObjectConfiguration final {
public:
    // IDA 0x60cbc0.
    [[nodiscard]] static std::uint8_t Load(
        CylinderObjectState& state, std::uint32_t document,
        ICylinderObjectConfigRuntime& runtime);
    // IDA 0x60d2a0.
    [[nodiscard]] static std::uint8_t Reset(
        CylinderObjectState& state, ICylinderObjectConfigRuntime& runtime);
};

}  // namespace pal4
