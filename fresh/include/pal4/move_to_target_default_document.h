#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct MoveToTargetAttributeTemporary final {
    std::array<std::uint32_t, 7u> words{};
};
static_assert(sizeof(MoveToTargetAttributeTemporary) == 0x1cu);

// Native XML document allocation, lookup and mutation for IDA 0x61f570.
class IMoveToTargetDefaultDocumentRuntime {
public:
    virtual ~IMoveToTargetDefaultDocumentRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeDocument(
        std::uint32_t address) = 0;
    virtual void SetName(std::uint32_t document, std::string_view name,
                         std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FindAttribute(
        std::uint32_t document, std::string_view key) = 0;
    virtual void SetString(std::uint32_t attribute, std::string_view value) = 0;
    virtual void SetInt(std::uint32_t attribute, std::int32_t value) = 0;
    virtual void SetFloat(std::uint32_t attribute, float value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeStringAttribute(
        MoveToTargetAttributeTemporary& temporary, std::string_view key,
        std::string_view value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeIntAttribute(
        MoveToTargetAttributeTemporary& temporary, std::string_view key,
        std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeFloatAttribute(
        MoveToTargetAttributeTemporary& temporary, std::string_view key,
        float value) = 0;
    virtual void AddAttribute(std::uint32_t document,
                              std::uint32_t attribute) = 0;
    virtual void DestroyAttribute(MoveToTargetAttributeTemporary& temporary) = 0;
    virtual void WriteFloat(std::uint32_t document,
                            std::string_view key, float value) = 0;
};

class MoveToTargetDefaultDocument final {
public:
    // IDA 0x61f570.
    [[nodiscard]] static std::uint32_t Create(
        IMoveToTargetDefaultDocumentRuntime& runtime);
};

}  // namespace pal4
