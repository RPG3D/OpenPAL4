#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct ConfigVector3 final {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

class IConfigVectorRuntime {
public:
    virtual ~IConfigVectorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetConfigValue(
        std::uint32_t config, std::string_view key) = 0;
    [[nodiscard]] virtual std::uint32_t FindAttributeNode(
        std::uint32_t config, std::string_view key) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetFloat(std::uint32_t attribute, float value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFloatAttribute(
        std::string_view name, float value) = 0;
    virtual void AddAttribute(std::uint32_t config,
                              std::uint32_t attribute) = 0;
    virtual void DestroyAttribute(std::uint32_t attribute) = 0;
    [[nodiscard]] virtual float ToFloat(std::uint32_t attribute,
                                         float fallback) = 0;
    [[nodiscard]] virtual std::uint8_t ReleaseNameBuffer() = 0;
};

class ConfigVectorRuntimeStub final : public IConfigVectorRuntime {
public:
    [[nodiscard]] std::uint32_t GetConfigValue(
        std::uint32_t, std::string_view) override { return 0u; }
    [[nodiscard]] std::uint32_t FindAttributeNode(
        std::uint32_t, std::string_view) override { return 0xffffffe4u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override {
        return 0u;
    }
    void SetFloat(std::uint32_t, float) override {}
    [[nodiscard]] std::uint32_t CreateFloatAttribute(
        std::string_view, float) override { return 0u; }
    void AddAttribute(std::uint32_t, std::uint32_t) override {}
    void DestroyAttribute(std::uint32_t) override {}
    [[nodiscard]] float ToFloat(std::uint32_t,
                                 float fallback) override { return fallback; }
    [[nodiscard]] std::uint8_t ReleaseNameBuffer() override { return 0u; }
};

class ConfigVector final {
public:
    // IDA 0x5fb150.
    [[nodiscard]] static std::uint8_t Set(
        std::uint32_t config, std::string_view name,
        const ConfigVector3& value, IConfigVectorRuntime& runtime);
    // IDA 0x5fb480.
    [[nodiscard]] static ConfigVector3& Get(
        ConfigVector3& output, std::uint32_t config,
        std::string_view name, IConfigVectorRuntime& runtime);
};

}  // namespace pal4
