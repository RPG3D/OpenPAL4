#include "pal4/combat_ending_window_level_up.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kRow = 0x1000u;
constexpr std::uint32_t kActor = 0x2000u;
constexpr std::uint32_t kHint = 0x3000u;
constexpr std::uint32_t kList = 0x4000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEndingWindowLevelUpRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::string> text;
    std::vector<std::string> calls;
    std::string hint_text;
    std::uint32_t effect_context{};
    bool change_slot_after_first_match{};
    std::uint32_t formatted_actor_count{};

    Runtime() {
        words[kRow + 0x48u] = 5u;
        words[kRow + 4u] = 0x1100u;
        words[kActor + 0x45cu] = 0x9000u;
        words[kActor + 0x4a8u] = 2u;
        words[kList + 4u] = 0x5000u;
        words[kList + 8u] = 0x500cu;
        words[0x5000u] = 0x6000u;
        words[0x5004u] = 0x6100u;
        words[0x5008u] = 0x6200u;
        words[0x6000u + 0x40u] = 4u;
        words[0x6100u + 0x40u] = 5u;
        words[0x6200u + 0x40u] = 3u;
        words[0x600cu] = 0x9010u;
        words[0x610cu] = 0x9020u;
        words[0x620cu] = 0x9030u;
        text[0x9000u] = "Yun Tianhe";
        text[0x9010u] = "first";
        text[0x9020u] = "second";
        text[0x9030u] = "third";
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == kRow + 0x48u, "score field address");
        words[address] = value;
        calls.emplace_back("score");
    }
    std::string ReadText(std::uint32_t address) override {
        return address == 0u ? std::string{} : text.at(address);
    }
    std::string GameString(std::uint32_t id) override {
        Check(id == 0xcau || id == 0xb8u, "game text id");
        return id == 0xcau ? "level-format" : "actor-format";
    }
    std::string FormatLevelNotice(std::string_view format,
                                  std::string_view actor_name,
                                  std::int32_t level) override {
        Check(format == "level-format" && actor_name == "Yun Tianhe",
              "level notice arguments");
        calls.emplace_back("format-level");
        return std::string(actor_name) + ":" + std::to_string(level);
    }
    std::string FormatActorNotice(std::string_view format,
                                  std::string_view actor_name) override {
        Check(format == "actor-format", "actor notice format");
        calls.emplace_back("format-actor");
        if (change_slot_after_first_match && ++formatted_actor_count == 1u)
            words[kActor + 0x4a8u] = 1u;
        return std::string(actor_name);
    }
    void SetWindowText(std::uint32_t window,
                       std::string_view value) override {
        Check(window == 0x1100u, "score text window");
        calls.emplace_back("text:" + std::string(value));
    }
    void SetHintFlag(std::uint32_t window, std::uint32_t flag) override {
        Check(window == kHint && flag == 0u, "hint flag arguments");
        calls.emplace_back("hint-flag");
    }
    void ShowHint(std::uint32_t window, std::string_view value,
                  std::uint32_t position,float duration, std::uint32_t first_flag,
                  std::uint32_t second_flag) override {
        Check(window == kHint && position==kRow+64u && duration == 3.0f &&
                  first_flag == 0u && second_flag == 0u,
              "hint display arguments");
        hint_text = value;
        calls.emplace_back("show-hint");
    }
    std::uint32_t CombatLevelUp(std::uint32_t actor) override {
        Check(actor == kActor, "level-up actor");
        calls.emplace_back("level-up");
        return 0x7000u;
    }
    void RegisterActorEvent(std::uint32_t event) override {
        Check(event == 0x7000u, "level-up event payload");
        calls.emplace_back("actor-event");
    }
    void PlaySound(std::string_view name, std::uint32_t mode) override {
        Check(mode == 1u, "combat sound mode");
        calls.emplace_back("sound:" + std::string(name));
    }
    std::uint32_t SpawnEffect(std::uint32_t actor,
                              std::string_view name,
                              std::uint32_t context) override {
        Check(actor == kActor && name == "EFF17", "level-up effect");
        effect_context = context;
        calls.emplace_back("effect");
        return 0x7100u;
    }
    void ResetEffectHandle(std::uint32_t handle) override {
        Check(handle == 0x7100u, "effect handle reset");
        calls.emplace_back("reset");
    }
};

void Apply(Runtime& runtime, std::int32_t level) {
    pal4::CombatEndingWindowLevelUp::Apply(
        kRow, level, kActor, kHint, kList, runtime);
}

void CheckGrowthGate() {
    Runtime runtime;
    Apply(runtime, 5);
    Apply(runtime, 4);
    Check(runtime.calls.empty() && runtime.words[kRow + 0x48u] == 5u,
          "equal and lower levels have no side effects");

    Runtime signed_level;
    signed_level.words[kRow + 0x48u] = 0xffffffffu;
    Apply(signed_level, 0);
    Check(signed_level.words[kRow + 0x48u] == 0u &&
              signed_level.calls[1] == "text:0",
          "level gate compares signed 32-bit values");
}

void CheckNoMatches() {
    Runtime runtime;
    Apply(runtime, 6);
    Check(runtime.words[kRow + 0x48u] == 6u &&
              runtime.hint_text == "Yun Tianhe:6" &&
              runtime.effect_context == 0x8fa274u,
          "level, plain hint and effect context");
    Check(runtime.calls == (std::vector<std::string>{
              "score", "text:6", "format-level", "hint-flag",
              "show-hint", "level-up", "actor-event", "sound:WA18.wav",
              "effect", "reset"}),
          "no-match path updates UI and plays only the base sound");
}

void CheckMultipleMatches() {
    Runtime runtime;
    runtime.words[0x6000u + 0x40u] = 6u;
    runtime.words[0x6200u + 0x40u] = 6u;
    Apply(runtime, 6);
    Check(runtime.hint_text == "Yun Tianhe:6<br/>first<br/>third",
          "all matching records append in list order");
    Check(runtime.calls == (std::vector<std::string>{
              "score", "text:6", "format-level", "format-actor",
              "format-actor", "hint-flag", "show-hint", "level-up",
              "actor-event", "sound:WA18.wav", "sound:WA17.wav",
              "effect", "reset"}),
          "matching records trigger one extra sound after the base sound");

    Runtime alternate_slot;
    alternate_slot.words[kActor + 0x4a8u] = 1u;
    alternate_slot.words[0x6000u + 0x3cu] = 6u;
    Apply(alternate_slot, 6);
    Check(alternate_slot.hint_text == "Yun Tianhe:6<br/>first",
          "record comparison uses the actor's selected 32-bit slot");

    Runtime changing_slot;
    changing_slot.change_slot_after_first_match = true;
    changing_slot.words[0x6000u + 0x40u] = 6u;
    changing_slot.words[0x6200u + 0x3cu] = 6u;
    Apply(changing_slot, 6);
    Check(changing_slot.hint_text == "Yun Tianhe:6<br/>first<br/>third",
          "each record rereads the actor's live slot index");
}

}  // namespace

int main() {
    try {
        CheckGrowthGate();
        CheckNoMatches();
        CheckMultipleMatches();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
