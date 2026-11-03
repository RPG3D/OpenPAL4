#pragma once

#include "pal4/effect_system_object_f.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IEffectSystemObjectFConfigRuntime {
public:
    virtual ~IEffectSystemObjectFConfigRuntime() = default;
    [[nodiscard]] virtual std::string ReadString(
        std::uint32_t document, std::string_view key,
        std::string_view fallback) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t document, std::string_view key, float fallback) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t document, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual std::uint32_t FindTexture(
        std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t LoadTexture(
        std::string_view path) = 0;
    virtual void DestroyTexture(std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateObject(
        std::uint32_t bytes) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool InitializeObject(
        std::uint32_t object, std::uint32_t texture,
        const std::array<float, 2>& size, std::uint8_t alpha_blend,
        std::uint8_t axis_billboard, std::uint8_t center) = 0;
    virtual void SetColor(std::uint32_t color_slot,
                          std::uint32_t rgba) = 0;
    virtual void InitializeTransform(std::uint32_t object) = 0;
    virtual void AssignDocument(EffectSystemObjectFState& state,
                                std::uint32_t document) = 0;
};

class EffectSystemObjectFConfigRuntimeStub final
    : public IEffectSystemObjectFConfigRuntime {
public:
    [[nodiscard]] std::string ReadString(
        std::uint32_t, std::string_view,
        std::string_view fallback) override { return std::string(fallback); }
    [[nodiscard]] float ReadFloat(
        std::uint32_t, std::string_view, float fallback) override {
        return fallback;
    }
    [[nodiscard]] std::int32_t ReadInt(
        std::uint32_t, std::string_view,
        std::int32_t fallback) override { return fallback; }
    [[nodiscard]] std::uint32_t FindTexture(
        std::string_view) override { return 0u; }
    [[nodiscard]] std::uint32_t LoadTexture(
        std::string_view) override { return 0u; }
    void DestroyTexture(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t AllocateObject(
        std::uint32_t) override { return 0u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] bool InitializeObject(
        std::uint32_t, std::uint32_t, const std::array<float, 2>&,
        std::uint8_t, std::uint8_t, std::uint8_t) override { return false; }
    void SetColor(std::uint32_t, std::uint32_t) override {}
    void InitializeTransform(std::uint32_t) override {}
    void AssignDocument(EffectSystemObjectFState&,
                        std::uint32_t) override {}
};

}  // namespace pal4
