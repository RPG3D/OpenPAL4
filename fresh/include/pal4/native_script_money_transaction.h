#pragma once
#include "pal4/script_money_transaction.h"
#include <array>
#include <string>

namespace pal4 {
class NativeInventorySession;
class IScriptInventoryChangeRuntime;
// Borrows the existing inventory, GameString lookup and hint/audio endpoints.
class NativeScriptMoneyTransaction final : public IScriptMoneyTransactionRuntime {
public:
    NativeScriptMoneyTransaction(NativeInventorySession&, IScriptInventoryChangeRuntime&);
    std::uint32_t Inventory() override;
    void AdjustMoney(std::uint32_t, std::int32_t) override;
    void InitializeText() override;
    std::uint32_t GameDatabase() override;
    std::uint32_t FindGameString(std::uint32_t, std::uint32_t) override;
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint32_t EmptyStringAddress() override;
    void FormatText(std::uint32_t, std::int32_t) override;
    std::uint32_t UiFrameManager() override;
    std::uint32_t CreateHintWindow(std::uint32_t) override;
    std::uint32_t FormattedTextAddress() override;
    void ShowHint(std::uint32_t, std::uint32_t, float, float, float,
                  std::uint32_t, std::uint32_t) override;
    std::uint32_t AudioManager() override;
    void PlaySound(std::uint32_t, std::uint8_t, std::string_view, std::uint32_t) override;
    std::int32_t CleanupText() noexcept override;
private:
    NativeInventorySession& inventory_;
    IScriptInventoryChangeRuntime& hints_;
    std::array<std::uint32_t, 2> format_{};
    std::string text_;
};
}
