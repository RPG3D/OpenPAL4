#include "pal4/role_state_window_player_info.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IRoleStateWindowPlayerRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> memory;
    std::map<std::uint32_t, std::string> texts;
    std::map<std::uint32_t, float> widths;
    std::map<std::uint32_t, std::int32_t> current{
        {4u, 42}, {5u, 20}, {7u, 80}, {8u, 35}, {9u, 40}};
    std::map<std::uint32_t, std::int32_t> base{{0u, 100}, {2u, 50}};
    std::uint32_t player = 0x10000u;
    std::uint32_t portrait = 0u;
    std::uint32_t image = 0u;
    std::uint32_t begin_count = 0u;
    std::uint32_t end_count = 0u;
    std::uint32_t threshold_calls = 0u;
    std::int32_t threshold = 75;
    bool throw_on_text = false;

    Runtime() {
        memory[player + 0xb08u] = 1u;
        memory[player + 0xa48u] = 2u;
        memory[player + 0x9fcu] = 0x20000u;
    }
    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x30000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::uint32_t index) override {
        Check(manager == 0x30000u && index == 3u,
              "player manager and index");
        calls.emplace_back("player");
        return player;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return memory[address];
    }
    void PreparePlayerData(std::uint32_t data,
                           std::uint32_t role_index) override {
        Check(data == player + 0x654u && role_index == 2u,
              "player data preparation arguments");
        calls.emplace_back("prepare");
    }
    void BeginTextBuffer() override {
        ++begin_count;
        calls.emplace_back("begin-text");
    }
    void EndTextBuffer() override {
        ++end_count;
        calls.emplace_back("end-text");
    }
    void SetImage(std::uint32_t window,
                  std::uint32_t selected) override {
        Check(window == 0x500cu, "portrait window");
        portrait = window;
        image = selected;
        calls.emplace_back("image");
    }
    std::string ConvertToUtf8(std::uint32_t address) override {
        calls.push_back("utf8:" + std::to_string(address));
        if (address == 0x20000u) return "Yun Tianhe";
        if (address == 0x20010u) return "Info one";
        if (address == 0x20020u) return "Info two";
        Check(address ==
                  pal4::RoleStateWindowPlayerInfo::kEmptyTextAddress,
              "null text uses empty global");
        return "";
    }
    std::int32_t ElementValue(std::uint32_t data,
                              std::uint32_t element) override {
        Check(data == player + 0x654u && element < 5u,
              "element selector");
        calls.push_back("element:" + std::to_string(element));
        return static_cast<std::int32_t>(element + 11u);
    }
    std::int32_t BaseValue(std::uint32_t data,
                           std::uint32_t field) override {
        Check(data == player + 0x654u, "base data pointer");
        calls.push_back("base:" + std::to_string(field));
        return base.at(field);
    }
    std::int32_t CurrentValue(std::uint32_t data,
                              std::uint32_t field) override {
        Check(data == player + 0x654u, "current data pointer");
        calls.push_back("current:" + std::to_string(field));
        return current.at(field);
    }
    std::uint32_t TriggerText(std::uint32_t trigger) override {
        Check(trigger == 0x60u || trigger == 0x61u,
              "GameDB trigger identifiers");
        calls.push_back("trigger:" + std::to_string(trigger));
        return trigger == 0x60u ? 0x20010u : 0x20020u;
    }
    std::int32_t NextLevelThreshold(std::uint32_t role_id,
                                    std::int32_t next_level) override {
        Check(role_id == 1003u && next_level == current.at(4u) + 1,
              "next-level lookup arguments");
        ++threshold_calls;
        calls.emplace_back("threshold");
        return threshold;
    }
    void SetText(std::uint32_t window,
                 const std::string& text) override {
        if (throw_on_text) throw std::runtime_error("text");
        texts[window] = text;
        calls.push_back("text:" + std::to_string(window));
    }
    void SetEnabled(std::uint32_t window, bool enabled) override {
        Check(!enabled && window >= 0x5050u && window <= 0x505cu,
              "four info controls disabled");
        calls.push_back("disable:" + std::to_string(window));
    }
    void SetWidth(std::uint32_t window, float width) override {
        widths[window] = width;
        calls.push_back("width:" + std::to_string(window));
    }
    void SetVisible(std::uint32_t window, bool value) override {
        calls.push_back("visible:" + std::to_string(window) + ":" +
                        (value ? "1" : "0"));
    }
    void SetImage(std::uint32_t window, std::string_view set,
                  std::string_view image_name) override {
        Check(set == "kuang0", "button imageset");
        calls.push_back("button-image:" + std::to_string(window) +
                        ":" + std::string(image_name));
    }
    void ClearImage(std::uint32_t window) override {
        calls.push_back("clear-image:" + std::to_string(window));
    }
};

pal4::RoleStateWindowState MakeState() {
    pal4::RoleStateWindowState state;
    state.Write(8u, 2u);
    for (std::uint32_t offset : {12u, 20u, 24u, 28u, 32u, 36u, 40u,
                                 48u, 52u, 56u, 60u, 64u, 68u,
                                 80u, 84u, 88u, 92u, 100u})
        state.Write(offset, 0x5000u + offset);
    state.Write(120u, 0x9000u);
    state.Write(124u, 0x9010u);
    state.Write(128u, 0x9020u);
    state.Write(132u, 0x9030u);
    state.Write(72u, 0x5048u);
    state.Write(76u, 0x504cu);
    state.Write(96u, 0x5060u);
    state.Write(104u, 0x43480000u); // 200.0f
    state.Write(108u, 0x42f00000u); // 120.0f
    state.Write(112u, 0x42b40000u); // 90.0f
    return state;
}

void CheckActiveResult(const pal4::RoleStateWindowState& state,
                       const Runtime& runtime) {
    Check(state.Read(0u) == 3u && runtime.image == 0x9020u &&
              runtime.texts.at(0x5064u) == "Yun Tianhe" &&
              runtime.texts.at(0x5014u) == "4\n0\n2\n \n ",
          "player index, portrait, name and level");
    for (std::uint32_t i = 0u; i < 5u; ++i)
        Check(runtime.texts.at(0x5000u + 24u + i * 4u) ==
                  std::to_string(11u + i),
              "element values map to five controls");
    Check(runtime.texts.at(0x5030u) == "80/100" &&
              runtime.texts.at(0x5034u) == "40/50" &&
              runtime.texts.at(0x5038u) == "35/100",
          "jing, shen and qi current/base text");
    Check(runtime.texts.at(0x5050u) == "Info one" &&
              runtime.texts.at(0x5058u) == "Info two" &&
              runtime.texts.at(0x5054u) == "20" &&
              runtime.texts.at(0x505cu) == "55",
          "GameDB labels, experience and next-level remainder");
    Check(std::fabs(runtime.widths.at(0x503cu) - 160.0f) < 0.001f &&
              std::fabs(runtime.widths.at(0x5040u) - 96.0f) < 0.001f &&
              std::fabs(runtime.widths.at(0x5044u) - 31.5f) < 0.001f,
          "HP, MP and rage widths");
    Check(runtime.begin_count == 1u &&
              runtime.end_count == 1u &&
              runtime.calls[runtime.calls.size() - 7u] ==
                  "visible:20492:1" &&
              runtime.calls[runtime.calls.size() - 6u] ==
                  "visible:20552:1" &&
              runtime.calls[runtime.calls.size() - 5u] ==
                  "visible:20556:0" &&
              runtime.calls[runtime.calls.size() - 4u] == "end-text" &&
              runtime.calls[runtime.calls.size() - 3u] ==
                  "visible:20576:1" &&
              runtime.calls[runtime.calls.size() - 2u] ==
                  "clear-image:20576" &&
              runtime.calls.back() ==
                  "button-image:20580:renwuanniud",
          "show, buffer cleanup, button-image order");
}

void TestActivePlayer() {
    auto state = MakeState();
    Runtime runtime;
    pal4::RoleStateWindowPlayerInfo::Update(
        state, 3u, runtime);
    CheckActiveResult(state, runtime);
    Check(runtime.threshold_calls == 1u,
          "next-level lookup for level below 99");

    auto max_state = MakeState();
    Runtime max_level;
    max_level.current[4u] = 99;
    pal4::RoleStateWindowPlayerInfo::Update(
        max_state, 3u, max_level);
    Check(max_level.texts.at(0x505cu) == "--" &&
              max_level.threshold_calls == 0u,
          "level 99 skips next-level lookup");

    auto low_state = MakeState();
    Runtime low_threshold;
    low_threshold.threshold = 5;
    pal4::RoleStateWindowPlayerInfo::Update(
        low_state, 3u, low_threshold);
    Check(low_threshold.texts.at(0x505cu) == "0",
          "negative experience remainder clamped to zero");
}

void TestMissingAndInactive() {
    auto state = MakeState();
    Runtime missing;
    missing.player = 0u;
    pal4::RoleStateWindowPlayerInfo::Update(
        state, 3u, missing);
    Check(state.Read(0u) == 3u &&
              missing.calls == std::vector<std::string>{
                  "manager", "player"},
          "missing player only caches selected index");

    state = MakeState();
    Runtime inactive;
    inactive.memory[inactive.player + 0xb08u] = 0u;
    pal4::RoleStateWindowPlayerInfo::Update(
        state, 3u, inactive);
    Check(inactive.calls == std::vector<std::string>{
              "manager", "player",
              "read:" + std::to_string(inactive.player + 0xb08u),
              "visible:20492:0", "visible:20552:0",
              "visible:20556:1", "visible:20576:1",
              "clear-image:20576",
              "button-image:20580:renwuanniud"} &&
              inactive.begin_count == 0u,
          "inactive player hides window and refreshes button image");
}

void TestNullNameAndFailureCleanup() {
    auto state = MakeState();
    Runtime null_name;
    null_name.memory[null_name.player + 0x9fcu] = 0u;
    pal4::RoleStateWindowPlayerInfo::Update(
        state, 3u, null_name);
    Check(null_name.texts.at(0x5064u).empty(),
          "null name converts global empty text");

    state = MakeState();
    Runtime failing;
    failing.throw_on_text = true;
    try {
        pal4::RoleStateWindowPlayerInfo::Update(
            state, 3u, failing);
        Check(false, "text failure expected");
    } catch (const std::runtime_error&) {
        Check(failing.begin_count == 1u && failing.end_count == 1u &&
                  failing.calls.back() == "end-text",
              "text buffer unwinds on CEGUI failure");
    }
}

}  // namespace

int main() {
    TestActivePlayer();
    TestMissingAndInactive();
    TestNullNameAndFailureCleanup();
}
