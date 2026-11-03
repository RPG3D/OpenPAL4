#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatActionConsoleWindowElementRuntime {
public:
    virtual ~ICombatActionConsoleWindowElementRuntime() = default;
    virtual void InitializeText() = 0;
    [[nodiscard]] virtual std::uint32_t CleanupText() = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FormatName(
        std::uint32_t prefix, std::string_view suffix) = 0;
    [[nodiscard]] virtual std::uint32_t ToUtf8(
        std::uint32_t formatted_name) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindowByName(
        std::uint32_t owner, std::uint32_t utf8_name) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::uint32_t owner, std::uint32_t formatted_name) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeClicked(
        std::uint32_t button, std::uint32_t callback) = 0;
    virtual void DestroyConnection(std::uint32_t connection,
                                   std::uint32_t flags) = 0;
    // Default preserves the recovered reference release. Native owners may
    // adopt the returned reference and disconnect it at their own Close.
    virtual void ReleaseConnection(std::uint32_t connection);
    virtual void SetAlwaysOnTop(std::uint32_t window, bool enabled) = 0;
};

class CombatActionConsoleWindowElements final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x845350u;
    static constexpr std::uint32_t kClickCallback = 0x541480u;

    // IDA 0x5415c0.
    [[nodiscard]] static std::uint32_t Setup(
        std::uint32_t window,
        ICombatActionConsoleWindowElementRuntime& runtime);
};

}  // namespace pal4
