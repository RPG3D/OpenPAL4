#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptMoneyTransactionRuntime {
public:
    virtual ~IScriptMoneyTransactionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    virtual void AdjustMoney(std::uint32_t inventory,
                             std::int32_t delta) = 0;
    virtual void InitializeText() = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t FindGameString(
        std::uint32_t table, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    virtual void FormatText(std::uint32_t format,
                            std::int32_t amount) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateHintWindow(
        std::uint32_t frames) = 0;
    [[nodiscard]] virtual std::uint32_t FormattedTextAddress() = 0;
    virtual void ShowHint(std::uint32_t window,
                          std::uint32_t text, float x, float y,
                          float scale, std::uint32_t mode,
                          std::uint32_t enabled) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    virtual void PlaySound(std::uint32_t audio, std::uint8_t show_hint,
                           std::string_view path, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t CleanupText() noexcept = 0;
};

class ScriptMoneyTransaction final {
public:
    // IDA 0x5d7530, giAddMoney.
    [[nodiscard]] static std::int32_t Add(
        std::int32_t amount, std::uint8_t show_hint,
        IScriptMoneyTransactionRuntime& runtime);
    // IDA 0x5d7620, giPayMoney.
    [[nodiscard]] static std::int32_t Pay(
        std::int32_t amount, std::uint8_t show_hint,
        IScriptMoneyTransactionRuntime& runtime);

private:
    [[nodiscard]] static std::int32_t Run(
        std::int32_t amount, std::uint8_t show_hint, bool adding,
        IScriptMoneyTransactionRuntime& runtime);
};

}  // namespace pal4
