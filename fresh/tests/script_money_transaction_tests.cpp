#include "pal4/script_money_transaction.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptMoneyTransactionRuntime {
    bool adding = true;
    std::int32_t amount = 12;
    std::uint8_t show_hint = 1u;
    std::uint32_t format = 0x5000u;
    std::uint32_t hint = 0x7000u;
    bool throw_format = false;
    std::vector<std::string> calls;

    std::uint32_t Inventory() override {
        calls.emplace_back("inventory");
        return 0x1000u;
    }
    void AdjustMoney(std::uint32_t inventory,
                     std::int32_t delta) override {
        const auto expected = adding ? amount : std::bit_cast<std::int32_t>(
            0u - static_cast<std::uint32_t>(amount));
        Check(inventory == 0x1000u && delta == expected,
              "money delta and inventory");
        calls.emplace_back("adjust");
    }
    void InitializeText() override { calls.emplace_back("init text"); }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        return 0x2000u;
    }
    std::uint32_t FindGameString(std::uint32_t table,
                                 std::uint32_t id) override {
        Check(table == 0x2034u && id == (adding ? 0xc2u : 0x10bu),
              "money format lookup");
        calls.emplace_back("find string");
        return 0x3000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x3004u, "format payload offset");
        calls.emplace_back("format pointer");
        return format;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty");
        return 0x4000u;
    }
    void FormatText(std::uint32_t resolved_format,
                    std::int32_t value) override {
        Check(resolved_format == (format == 0u ? 0x4000u : format) &&
                  value == amount, "formatted money amount");
        calls.emplace_back("format text");
        if (throw_format) throw std::runtime_error("format failure");
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("frames");
        return 0x6000u;
    }
    std::uint32_t CreateHintWindow(std::uint32_t frames) override {
        Check(frames == 0x6000u, "hint frame manager");
        calls.emplace_back("create hint");
        return hint;
    }
    std::uint32_t FormattedTextAddress() override {
        calls.emplace_back("hint text");
        return 0x8000u;
    }
    void ShowHint(std::uint32_t window, std::uint32_t text,
                  float x, float y, float scale,
                  std::uint32_t mode, std::uint32_t enabled) override {
        Check(window == hint && text == 0x8000u && x == 0.5f &&
                  y == 0.15f && scale == 1.0f && mode == 0u &&
                  enabled == 1u, "money hint geometry and flags");
        calls.emplace_back("show hint");
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio");
        return 0x9000u;
    }
    void PlaySound(std::uint32_t audio, std::uint8_t flag,
                   std::string_view path, std::uint32_t mode) override {
        Check(audio == 0x9000u && flag == show_hint &&
                  path == "gamedata\\PALSound\\WA40.wav" && mode == 1u,
              "add-money sound path and original flag");
        calls.emplace_back("sound");
    }
    std::int32_t CleanupText() noexcept override {
        calls.emplace_back("cleanup");
        return -7;
    }
};

void TransactionBranches() {
    Runtime add;
    Check(pal4::ScriptMoneyTransaction::Add(12, 1u, add) == -7 &&
              add.calls == std::vector<std::string>{
                  "inventory", "adjust", "init text", "database",
                  "find string", "format pointer", "format text", "frames",
                  "create hint", "hint text", "show hint", "audio",
                  "sound", "cleanup"},
          "add money formats, shows hint, plays sound and cleans text");
    Runtime add_quiet;
    add_quiet.show_hint = 0u;
    add_quiet.format = 0u;
    Check(pal4::ScriptMoneyTransaction::Add(12, 0u, add_quiet) == -7 &&
              add_quiet.calls == std::vector<std::string>{
                  "inventory", "adjust", "init text", "database",
                  "find string", "format pointer", "empty", "format text",
                  "audio", "sound", "cleanup"},
          "add without hint still formats, plays sound, then cleans");
    Runtime pay;
    pay.adding = false;
    Check(pal4::ScriptMoneyTransaction::Pay(12, 1u, pay) == -7 &&
              pay.calls == std::vector<std::string>{
                  "inventory", "adjust", "init text", "database",
                  "find string", "format pointer", "format text", "frames",
                  "create hint", "hint text", "show hint", "cleanup"},
          "pay money uses negative delta and no sound");
    Runtime no_window;
    no_window.adding = false;
    no_window.hint = 0u;
    Check(pal4::ScriptMoneyTransaction::Pay(12, 1u, no_window) == -7 &&
              no_window.calls == std::vector<std::string>{
                  "inventory", "adjust", "init text", "database",
                  "find string", "format pointer", "format text", "frames",
                  "create hint", "cleanup"},
          "missing hint window skips display but cleans text");
    Runtime exceptional;
    exceptional.throw_format = true;
    try {
        (void)pal4::ScriptMoneyTransaction::Add(12, 1u, exceptional);
        throw std::runtime_error("expected formatting failure");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "format failure" &&
                  exceptional.calls.back() == "cleanup",
              "format exception cleans text before propagation");
    }
    Runtime minimum;
    minimum.adding = false;
    minimum.amount = std::numeric_limits<std::int32_t>::min();
    minimum.show_hint = 0u;
    Check(pal4::ScriptMoneyTransaction::Pay(
              minimum.amount, 0u, minimum) == -7 &&
              minimum.calls.back() == "cleanup",
          "pay preserves 32-bit negation wrap for minimum integer");
}

}  // namespace

int main() {
    try {
        TransactionBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
