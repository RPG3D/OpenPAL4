#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

class IPalConsoleArenaLoadRuntime {
public:
    virtual ~IPalConsoleArenaLoadRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::int32_t AddOutputLine(
        std::uint32_t console, std::string_view format) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeXmlDocument() = 0;
    // The tree lookup includes temporary key allocation and release.
    [[nodiscard]] virtual std::optional<std::uint32_t> FindTreeAttribute(
        std::uint32_t document, std::string_view key) = 0;
    [[nodiscard]] virtual std::optional<std::uint32_t> FindConfigAttribute(
        std::uint32_t document, std::string_view key) = 0;
    virtual void SetInt(std::uint32_t attribute, std::int32_t value) = 0;
    virtual void SetString(std::uint32_t attribute,
                           std::string_view value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeIntAttribute(
        std::string_view key, std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeStringAttribute(
        std::string_view key, std::string_view value) = 0;
    virtual void AddAttribute(std::uint32_t document,
                              std::uint32_t attribute) = 0;
    virtual void DestroyAttribute(std::uint32_t attribute) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    virtual void InitializeStateAndUi(std::uint32_t game,
                                      std::uint32_t state_id,
                                      std::uint32_t document) = 0;
    [[nodiscard]] virtual std::int32_t DestroyXmlDocument(
        std::uint32_t document) = 0;
};

class PalConsoleArenaLoadCommand final {
public:
    // IDA 0x5bb550, command "LA".
    [[nodiscard]] static std::int32_t Execute(
        const char* command, IPalConsoleArenaLoadRuntime& runtime);
};

}  // namespace pal4
