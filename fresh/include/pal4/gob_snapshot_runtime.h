#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGobSnapshotRuntime {
public:
    virtual ~IGobSnapshotRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t memory) = 0;
    virtual void InitializeNameRecord(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseNameRecord(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::string ReadNameRecord(
        std::uint32_t record) = 0;
    virtual void AssignNameRecord(std::uint32_t record,
                                  std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXmlDocument(
        std::string_view root) = 0;
    virtual void WriteXmlString(std::uint32_t document,
                                std::string_view key,
                                std::string_view value) = 0;
    virtual void WriteXmlInt(std::uint32_t document,
                             std::string_view key,
                             std::int32_t value) = 0;
    virtual void FlushXml(std::uint32_t document,
                          std::uint32_t stream) = 0;
    [[nodiscard]] virtual bool ReadXml(std::uint32_t document,
                                       std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::string ReadXmlString(
        std::uint32_t document, std::string_view key,
        std::string_view fallback) = 0;
    [[nodiscard]] virtual std::int32_t ReadXmlInt(
        std::uint32_t document, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyXmlDocument(
        std::uint32_t document) = 0;
};

}  // namespace pal4
