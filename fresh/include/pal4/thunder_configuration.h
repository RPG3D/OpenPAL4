#pragma once

#include "pal4/thunder_object.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IThunderConfigurationRuntime {
public:
    virtual ~IThunderConfigurationRuntime() = default;
    [[nodiscard]] virtual std::string ReadString(
        std::uint32_t document, std::string_view key,
        std::string_view fallback) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t document, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t document, std::string_view key,
        float fallback) = 0;
    virtual void SetTargetName(ThunderObjectState& state,
                               std::string_view name) = 0;
    virtual void ResolveTargetPosition(
        std::uint32_t owner, std::string_view target_name,
        std::array<float, 3u>& position) = 0;
    [[nodiscard]] virtual std::uint32_t FindTexture(
        std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t LoadTexture(
        std::string_view path) = 0;
    [[nodiscard]] virtual bool InitializeVisual(
        ThunderObjectState& state, std::uint32_t texture,
        float fps, std::uint8_t mode) = 0;
    virtual void UpdateFrame(ThunderObjectState& state) = 0;
    virtual void DestroyTexture(std::uint32_t texture) = 0;
    virtual void ReleaseTexturePath(std::string_view path) = 0;
};

class ThunderConfigurationRuntimeStub final : public IThunderConfigurationRuntime {
public:
    [[nodiscard]] std::string ReadString(
        std::uint32_t, std::string_view,
        std::string_view fallback) override { return std::string(fallback); }
    [[nodiscard]] std::int32_t ReadInt(
        std::uint32_t, std::string_view,
        std::int32_t fallback) override { return fallback; }
    [[nodiscard]] float ReadFloat(
        std::uint32_t, std::string_view,
        float fallback) override { return fallback; }
    void SetTargetName(ThunderObjectState&, std::string_view) override {}
    void ResolveTargetPosition(std::uint32_t, std::string_view,
                               std::array<float, 3u>&) override {}
    [[nodiscard]] std::uint32_t FindTexture(std::string_view) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t LoadTexture(std::string_view) override {
        return 0u;
    }
    [[nodiscard]] bool InitializeVisual(
        ThunderObjectState&, std::uint32_t, float, std::uint8_t) override {
        return false;
    }
    void UpdateFrame(ThunderObjectState&) override {}
    void DestroyTexture(std::uint32_t) override {}
    void ReleaseTexturePath(std::string_view) override {}
};

class ThunderConfiguration final {
public:
    // IDA 0x61b890.
    [[nodiscard]] static std::uint8_t Load(
        ThunderObjectState& state, std::uint32_t document,
        IThunderConfigurationRuntime& runtime);
};

}  // namespace pal4
