#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct BillboardAttributeTemporary final {
    std::array<std::uint32_t, 7> words{};
};
static_assert(sizeof(BillboardAttributeTemporary) == 0x1c);

class IBillboardDefaultDocumentRuntime {
public:
    virtual ~IBillboardDefaultDocumentRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeDocument(
        std::uint32_t address) = 0;
    virtual void SetName(std::uint32_t document, std::string_view name,
                         std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FindAttribute(
        std::uint32_t document, std::string_view key) = 0;
    virtual void SetString(std::uint32_t attribute,
                           std::string_view value) = 0;
    virtual void SetFloat(std::uint32_t attribute, float value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeStringAttribute(
        BillboardAttributeTemporary& temporary, std::string_view key,
        std::string_view value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeFloatAttribute(
        BillboardAttributeTemporary& temporary, std::string_view key,
        float value) = 0;
    virtual void AddAttribute(std::uint32_t document,
                              std::uint32_t attribute) = 0;
    virtual void DestroyAttribute(
        BillboardAttributeTemporary& temporary) = 0;
    virtual void WriteInt(std::uint32_t document, std::string_view key,
                          std::int32_t value) = 0;
};

class BillboardDefaultDocument final {
public:
    // IDA 0x605010. The spelling of the XML name and keys is native.
    [[nodiscard]] static std::uint32_t Create(
        IBillboardDefaultDocumentRuntime& runtime);
};

}  // namespace pal4
