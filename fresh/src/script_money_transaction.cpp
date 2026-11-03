#include "pal4/script_money_transaction.h"

#include <bit>

namespace pal4 {

std::int32_t ScriptMoneyTransaction::Run(
    std::int32_t amount, std::uint8_t show_hint, bool adding,
    IScriptMoneyTransactionRuntime& runtime) {
    const auto delta = adding ? amount : std::bit_cast<std::int32_t>(
        0u - static_cast<std::uint32_t>(amount));
    runtime.AdjustMoney(runtime.Inventory(), delta);
    runtime.InitializeText();
    try {
        const auto record = runtime.FindGameString(
            runtime.GameDatabase() + 0x34u, adding ? 0xc2u : 0x10bu);
        auto format = runtime.ReadWord(record + 4u);
        if (format == 0u) format = runtime.EmptyStringAddress();
        runtime.FormatText(format, amount);
        if (show_hint != 0u) {
            const auto hint = runtime.CreateHintWindow(runtime.UiFrameManager());
            if (hint != 0u)
                runtime.ShowHint(hint, runtime.FormattedTextAddress(),
                                 0.5f, 0.15f, 1.0f, 0u, 1u);
        }
        if (adding)
            runtime.PlaySound(runtime.AudioManager(), show_hint,
                              "gamedata\\PALSound\\WA40.wav", 1u);
    } catch (...) {
        (void)runtime.CleanupText();
        throw;
    }
    return runtime.CleanupText();
}

std::int32_t ScriptMoneyTransaction::Add(
    std::int32_t amount, std::uint8_t show_hint,
    IScriptMoneyTransactionRuntime& runtime) {
    return Run(amount, show_hint, true, runtime);
}

std::int32_t ScriptMoneyTransaction::Pay(
    std::int32_t amount, std::uint8_t show_hint,
    IScriptMoneyTransactionRuntime& runtime) {
    return Run(amount, show_hint, false, runtime);
}

}  // namespace pal4
