#include "pal4/combat_ending_score_updater.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::uint32_t kDialog = 0x10000u;
constexpr std::uint32_t kResult = 0x20000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEndingScoreRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> texts;
    std::map<std::uint32_t, std::uint32_t> images;
    std::map<std::uint32_t, float> widths;
    std::map<std::uint32_t, float> positions;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> actors;
    std::size_t cursor = 0u;
    bool fail_text = false;
    bool released = false;

    Runtime() {
        for (std::uint32_t i = 0u; i < 5u; ++i)
            words[kResult + 4u * i] = 20u + i;
        words[kResult + 8u] = 2u;
        for (std::uint32_t i = 0u; i < 4u; ++i) {
            words[kResult + 72u + 40u * i] = 1000u + i;
            words[kDialog + 648u + 4u * i] = 800u + i;
        }
        for (std::uint32_t i = 0u; i < 5u; ++i)
            words[kDialog + 664u + 4u * i] = 900u + i;
        words[kDialog + 64u] = 101u;
        words[kDialog + 68u] = 102u;
        words[kDialog + 92u] = 103u;
        for (std::uint32_t i = 0u; i < 4u; ++i) {
            const auto row = kDialog + 116u + 76u * i;
            for (std::uint32_t offset = 0u; offset <= 44u; offset += 4u)
                words[row + offset] = 10000u + 100u * i + offset;
            words[row + 48u] = std::bit_cast<std::uint32_t>(100.0f);
            words[row + 52u] = std::bit_cast<std::uint32_t>(120.0f);
            words[row + 56u] = std::bit_cast<std::uint32_t>(80.0f);
        }
    }

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    void CopyTreeData(std::uint32_t to, std::uint32_t from) override {
        Check((to == kDialog + 440u && from == kResult + 20u) ||
                  (to == kDialog + 456u && from == kResult + 36u),
              "tree copy offsets");
        words[to + 12u] = words[from + 12u];
        calls.push_back("tree:" + std::to_string(to));
    }
    void CopyPlayerData(std::uint32_t to, std::uint32_t from) override {
        words[to + 20u] = words[from + 20u];
        calls.push_back("player:" + std::to_string(to));
    }
    std::uint32_t IconImage(std::uint32_t slot) override {
        calls.push_back("icon:" + std::to_string(slot));
        return 700u + slot;
    }
    std::string GameString(std::uint32_t id) override {
        calls.push_back("string:" + std::to_string(id));
        return id == 0xc2u ? "money:%d" : "grade:" + std::to_string(id);
    }
    std::string ToUtf8(std::string_view text) override {
        return std::string(text);
    }
    std::string FormatMoney(std::string_view format, std::int32_t value) override {
        Check(format == "money:%d", "money format");
        return "money:" + std::to_string(value);
    }
    void SetImage(std::uint32_t window, std::uint32_t image) override {
        images[window] = image;
        calls.push_back("image:" + std::to_string(window));
    }
    void SetText(std::uint32_t window, std::string_view text) override {
        if (fail_text) throw std::runtime_error("text failed");
        texts[window] = text;
        calls.push_back("text:" + std::to_string(window));
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(visible, "row is shown");
        calls.push_back("visible:" + std::to_string(window));
    }
    float GetHeight(std::uint32_t window) override {
        calls.push_back("height:" + std::to_string(window));
        return 31.5f;
    }
    void SetYPosition(std::uint32_t window, float y) override {
        positions[window] = y;
    }
    void SetWidth(std::uint32_t window, float width) override {
        widths[window] = width;
    }
    std::uint32_t BeginActors() override {
        calls.emplace_back("begin");
        return 0x90000u;
    }
    bool ActorsDone(std::uint32_t iterator) override {
        Check(iterator == 0x90000u, "iterator handle");
        return cursor == actors.size();
    }
    std::uint32_t CurrentActor(std::uint32_t iterator) override {
        Check(iterator == 0x90000u, "current handle");
        return actors.at(cursor);
    }
    void AdvanceActor(std::uint32_t iterator) override {
        Check(iterator == 0x90000u, "advance handle");
        ++cursor;
        calls.emplace_back("advance");
    }
    void ReleaseActors(std::uint32_t iterator) override {
        Check(iterator == 0x90000u, "release handle");
        released = true;
        calls.emplace_back("release");
    }
    std::int32_t ActorBaseStat(std::uint32_t, std::uint32_t stat) override {
        calls.push_back("base:" + std::to_string(stat));
        return stat == 0u ? 600 : 400;
    }
    std::int32_t ActorCurrentStat(std::uint32_t,
                                  std::uint32_t stat) override {
        calls.push_back("current:" + std::to_string(stat));
        return stat == 7u ? 520 : stat == 9u ? 300 : 80;
    }
    void UpdateScoreTextPart2(std::uint32_t dialog) override {
        Check(dialog == kDialog, "part2 dialog");
        calls.emplace_back("part2");
    }
    void UpdateScoreText(std::uint32_t dialog) override {
        Check(dialog == kDialog, "score text dialog");
        calls.emplace_back("score-text");
    }
    void ShowEndTaskScore(std::uint32_t dialog) override {
        Check(dialog == kDialog, "show dialog");
        calls.emplace_back("show");
    }
    int CleanupTextBuffer() override {
        calls.emplace_back("cleanup");
        return 37;
    }
};

std::size_t Find(const Runtime& runtime, std::string_view call) {
    for (std::size_t i = 0u; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing call: " + std::string(call));
}

void EmptyAndScoreBounds() {
    for (const auto score : {0u, 2u, 4u, 5u, 0xffffffffu}) {
        Runtime runtime;
        runtime.words[kResult + 8u] = score;
        const auto answer = pal4::CombatEndingScoreUpdater::Update(
            kDialog, kResult, runtime);
        Check(answer == 37 && runtime.released, "return and iterator release");
        for (std::uint32_t i = 0u; i < 5u; ++i)
            Check(runtime.words[kDialog + 420u + 4u * i] ==
                      runtime.words[kResult + 4u * i], "header copy");
        for (std::uint32_t i = 0u; i < 4u; ++i) {
            Check(runtime.words[kDialog + 632u + 4u * i] == 700u + i,
                  "four refreshed icons");
            Check(runtime.words[kDialog + 492u + 40u * i] == 1000u + i,
                  "four player records");
        }
        Check(runtime.words[kDialog + 716u] == 0u, "page reset");
        Check(runtime.images.contains(101u) == (score <= 4u) &&
                  runtime.texts.contains(102u) == (score <= 4u),
              "unsigned score range");
        if (score <= 4u)
            Check(runtime.images[101u] == 900u + score &&
                      runtime.texts[102u] ==
                          "grade:" + std::to_string(0xc3u + score),
                  "score image and localized text");
        Check(runtime.texts[103u] == "money:20", "money text");
        Check(Find(runtime, "tree:" + std::to_string(kDialog + 440u)) <
                  Find(runtime, "tree:" + std::to_string(kDialog + 456u)) &&
                  Find(runtime, "player:" + std::to_string(kDialog + 472u)) <
                      Find(runtime, "icon:0") &&
                  Find(runtime, "show") < Find(runtime, "release") &&
                  Find(runtime, "release") < Find(runtime, "cleanup"),
              "copy, presentation and release order");
    }
}

void ActorRowsAndOptionalTrees() {
    Runtime runtime;
    runtime.actors = {0x30000u, 0x31000u, 0x32000u};
    runtime.words[0x30000u + 0x4a8u] = 4u;
    runtime.words[0x31000u + 0x4a8u] = 3u;
    runtime.words[0x32000u + 0x4a8u] = 0u;
    runtime.words[kResult + 32u] = 1u;
    runtime.words[kResult + 48u] = 1u;
    const auto answer = pal4::CombatEndingScoreUpdater::Update(
        kDialog, kResult, runtime);
    Check(answer == 37 && runtime.cursor == 3u, "all actors traversed");
    const auto first = kDialog + 116u;
    const auto second = first + 76u;
    Check(runtime.words[first + 72u] == 1003u &&
              runtime.words[second + 72u] == 1000u &&
              runtime.words[first + 2u * 76u + 72u] == 0u,
          "valid slots pack rows in encounter order");
    Check(runtime.images[10000u + 40u] == 703u &&
              runtime.images[10000u + 44u] == 803u &&
              runtime.positions[10000u] == -31.5f,
          "row visibility, images and Y position");
    Check(runtime.texts[10000u + 4u] == "1003" &&
              runtime.texts[10000u + 20u] == "--" &&
              runtime.texts[10000u + 24u] == "--" &&
              runtime.texts[10000u + 8u] == "520/600" &&
              runtime.texts[10000u + 12u] == "300/400" &&
              runtime.texts[10000u + 16u] == "80/100",
          "row texts retain full actor stat width");
    Check(std::fabs(runtime.widths[10000u + 28u] - 86.66667f) < 0.001f &&
              std::fabs(runtime.widths[10000u + 32u] - 90.0f) < 0.001f &&
              std::fabs(runtime.widths[10000u + 36u] - 64.0f) < 0.001f,
          "HP, MP and rage widths");
    Check(Find(runtime, "base:0") < Find(runtime, "current:7") &&
              Find(runtime, "base:2") < Find(runtime, "current:9") &&
              Find(runtime, "part2") < Find(runtime, "score-text") &&
              Find(runtime, "score-text") < Find(runtime, "show"),
          "stat and conditional score ordering");
}

void FailureReleasesIterator() {
    Runtime runtime;
    runtime.actors = {0x31000u};
    runtime.words[0x31000u + 0x4a8u] = 1u;
    runtime.words[kResult + 8u] = 5u;
    runtime.fail_text = true;
    bool failed = false;
    try {
        (void)pal4::CombatEndingScoreUpdater::Update(kDialog, kResult, runtime);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.released &&
              Find(runtime, "release") == runtime.calls.size() - 1u,
          "iterator released when CEGUI text fails");
}

}  // namespace

int main() {
    try {
        EmptyAndScoreBounds();
        ActorRowsAndOptionalTrees();
        FailureReleasesIterator();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
