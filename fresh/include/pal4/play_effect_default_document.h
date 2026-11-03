#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PlayEffectAttributeTemporary final {
    std::array<std::uint32_t, 7u> words{};
};
static_assert(sizeof(PlayEffectAttributeTemporary) == 0x1cu);

// Native XML services for PlayEffectTask defaults at IDA 0x61fe30.
class IPlayEffectDefaultDocumentRuntime {
public:
    virtual ~IPlayEffectDefaultDocumentRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeDocument(
        std::uint32_t address) = 0;
    virtual void SetName(std::uint32_t document, std::string_view name,
                         std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FindAttribute(
        std::uint32_t document, std::string_view key) = 0;
    [[nodiscard]] virtual std::uint32_t FindTreeAttribute(
        std::uint32_t document, std::string_view key) = 0;
    virtual void SetString(std::uint32_t attribute, std::string_view value) = 0;
    virtual void SetInt(std::uint32_t attribute, std::int32_t value) = 0;
    virtual void SetFloat(std::uint32_t attribute, float value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeStringAttribute(
        PlayEffectAttributeTemporary& temporary, std::string_view key,
        std::string_view value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeIntAttribute(
        PlayEffectAttributeTemporary& temporary, std::string_view key,
        std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeFloatAttribute(
        PlayEffectAttributeTemporary& temporary, std::string_view key,
        float value) = 0;
    virtual void AddAttribute(std::uint32_t document,
                              std::uint32_t attribute) = 0;
    virtual void DestroyAttribute(PlayEffectAttributeTemporary& temporary) = 0;
};

class PlayEffectDefaultDocument final {
public:
    // IDA 0x61fe30.
    [[nodiscard]] static std::uint32_t Create(
        IPlayEffectDefaultDocumentRuntime& runtime);
};

}  // namespace pal4
