#include "pal4/combat_console.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kConsole = 0x5000u;
constexpr std::uint32_t kMainWindow = 0x9000u;

struct Runtime final : pal4::ICombatConsoleRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::vector<std::pair<std::string, std::string>> icons;
    pal4::CombatConsolePosition position{100.0f, 200.0f};
    float width = 300.0f;
    std::uint32_t legacy_count = 0u;
    std::uint32_t direct_count = 0u;

    void LoadLayout(std::uint32_t console, std::string_view name,
                    std::uint32_t first, std::uint32_t second) override {
        Check(console == kConsole && name == "combat.layout" &&
                  first == 0u && second == 0u,
              "layout arguments");
        calls.emplace_back("layout");
        words[kConsole + 36u] = 0x7000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(words.count(address) != 0u, "expected word read");
        if (address == kConsole + 212u) {
            Check(words.at(address) == kMainWindow, "main window pointer");
            calls.emplace_back("read-main");
        }
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(bytes.count(address) != 0u, "expected reference byte read");
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        if (address == kConsole)
            calls.emplace_back("vtable");
        else if (address == kConsole + 40u)
            calls.emplace_back("field-vtable");
        else if (address == kConsole + 52u)
            calls.emplace_back("geometry");
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        if (address == kConsole + 292u) {
            Check(value == 0u, "array first-byte initialization");
            calls.emplace_back("array-byte");
        } else {
            Check(bytes.count(address) != 0u, "reference byte decrement");
            bytes[address] = value;
            calls.emplace_back("decrement-reference");
        }
    }
    void Free(std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free:" + std::to_string(address));
    }
    std::uint32_t DestroyBase(std::uint32_t console) override {
        Check(console == kConsole, "base destructor target");
        calls.emplace_back("base-destruct");
        return 0xdeadbeefu;
    }
    void SetTextErrorFlag(std::uint32_t record,
                          std::uint8_t flag) override {
        if (record == kConsole + 276u) {
            Check(flag == 0u, "reset-command text flag");
            calls.emplace_back("text-flag");
        } else {
            Check(record >= 0xa000u && record < 0xa030u &&
                      (record - 0xa000u) % 16u == 0u && flag == 1u,
                  "array record release");
            calls.emplace_back("release-entry");
        }
    }
    void SetText(std::uint32_t record, std::string_view text) override {
        Check(record == kConsole + 276u && text == "ResetCombatIcon",
              "reset-command string");
        calls.emplace_back("reset-text");
    }
    std::uint32_t FindWindowByName(std::string_view name) override {
        if (name == "CombatMainWindow/StaticActionSequence") {
            calls.emplace_back("main-window");
            return kMainWindow;
        }
        Check(name.starts_with("CombatMainWindow/StaticRole") ||
                  name.starts_with("CombatMainWindow/StaticMonster"),
              "dynamic image lookup name");
        return 0x9100u + static_cast<std::uint32_t>(calls.size()) * 0x100u;
    }
    std::uint32_t FindWindow(std::uint32_t console,
                             std::string_view name) override {
        Check(console == kConsole &&
                  name.starts_with("CombatMainWindow/"),
              "fixed control lookup");
        if (name == "CombatMainWindow/EditMonsterInfo") return 0x8800u;
        return 0x8000u;
    }
    void SetAutoBreak(std::uint32_t edit, bool enabled) override {
        Check(edit == 0x8800u && !enabled, "edit auto-break disabled");
    }
    std::string ConvertToUtf8(std::string_view source) override {
        return std::string(source);
    }
    void SetChildVisible(std::uint32_t child, bool visible) override {
        Check(child != 0u && !visible, "image starts hidden");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 24u, "mouse callback size");
        return 0xc000u;
    }
    std::uint32_t MouseClickEvent() override { return 0xd000u; }
    void SubscribeClick(std::uint32_t root, std::uint32_t event,
                        std::uint32_t& callback) override {
        Check(root == 0x7000u && event == 0xd000u &&
                  callback == 0xc000u,
              "root mouse-click subscription");
    }
    void ReleaseCallback(std::uint32_t callback) override {
        Check(callback == 0xc000u, "temporary callback release");
    }
    pal4::CombatConsolePosition GetPosition(
        std::uint32_t window, std::uint32_t mode) override {
        Check(window == kMainWindow && mode == 1u,
              "main window position in pixel mode");
        calls.emplace_back("position");
        return position;
    }
    float GetWidth(std::uint32_t window, std::uint32_t mode) override {
        Check(window == kMainWindow && mode == 1u,
              "main window width in pixel mode");
        calls.emplace_back("width");
        return width;
    }
    void AppendLegacyText(std::uint32_t array,
                          std::string_view text) override {
        Check(array == kConsole + 292u, "legacy array address");
        ++legacy_count;
        icons.emplace_back("legacy", std::string(text));
        calls.emplace_back("legacy");
    }
    void AppendArrayText(std::uint32_t array,
                         std::string_view text) override {
        Check(array == kConsole + 292u, "direct array address");
        ++direct_count;
        icons.emplace_back("direct", std::string(text));
        calls.emplace_back("direct");
    }
};

std::size_t Position(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void ConstructionFieldsAndGeometry() {
    Runtime runtime;
    Check(pal4::CombatConsoleWindow::Construct(kConsole, "combat.layout", runtime) ==
              kConsole,
          "constructor returns object");
    for (std::uint32_t index = 0u; index < 19u; ++index) {
        Check(runtime.words.at(kConsole + 60u + index * 8u) == 0u &&
                  runtime.words.at(kConsole + 64u + index * 8u) == 0u,
              "all 19 event pairs start empty");
    }
    Check(runtime.words.at(kConsole) == pal4::CombatConsoleWindow::kVtable &&
              runtime.words.at(kConsole + 40u) ==
                  pal4::CombatConsoleWindow::kFieldVtable &&
              runtime.words.at(kConsole + 268u) == 0u &&
              runtime.words.at(kConsole + 272u) == 0u &&
              runtime.words.at(kConsole + 296u) == 0u &&
              runtime.words.at(kConsole + 300u) == 0u &&
              runtime.words.at(kConsole + 304u) == 0u,
          "vtable and member initialization");
    Check(std::bit_cast<float>(runtime.words.at(kConsole + 44u)) == 90.0f &&
              std::bit_cast<float>(runtime.words.at(kConsole + 48u)) == 185.0f &&
              std::bit_cast<float>(runtime.words.at(kConsole + 52u)) == 345.0f &&
              std::bit_cast<float>(runtime.words.at(kConsole + 56u)) == 185.0f,
          "adjusted main-window rectangle");
    Check(Position(runtime, "layout") < Position(runtime, "text-flag") &&
              Position(runtime, "reset-text") <
                  Position(runtime, "main-window") &&
              Position(runtime, "main-window") <
                  Position(runtime, "position") &&
              Position(runtime, "geometry") < Position(runtime, "legacy"),
          "layout, text, window, geometry and icon order");
}

void AllIconRecords() {
    Runtime runtime;
    (void)pal4::CombatConsoleWindow::Construct(kConsole, "combat.layout", runtime);
    constexpr std::array<std::string_view, 28> names{{
        "ding", "feng", "jing", "mian", "luan", "kuang", "shui",
        "huo", "lei", "fen", "tu", "jin", "bi", "jie", "ying",
        "ning", "bi2", "sheng", "wu1", "fang1", "su1", "yun1",
        "ling1", "wu2", "fang2", "su2", "yun2", "ling2"}};
    Check(runtime.icons.size() == 29u && runtime.legacy_count == 22u &&
              runtime.direct_count == 7u && runtime.icons.front().second.empty(),
          "empty plus 28 icon entries and insertion variants");
    for (std::size_t index = 0u; index < names.size(); ++index) {
        const auto& entry = runtime.icons[index + 1u];
        const auto expected = "<image set=kuang0 name=" +
                              std::string(names[index]) + "/>";
        Check(entry.first == (index < 21u ? "legacy" : "direct") &&
                  entry.second == expected,
              "IDA icon markup and order");
    }
}

void DestructionAndReferenceCounts() {
    for (const std::uint8_t reference : {0u, 1u, 0xffu}) {
        Runtime runtime;
        runtime.words[kConsole + 296u] = 0xa000u;
        runtime.words[kConsole + 300u] = 0xa030u;
        runtime.words[kConsole + 304u] = 0xa050u;
        runtime.words[kConsole + 280u] = 0xb000u;
        runtime.words[kConsole + 284u] = 15u;
        runtime.words[kConsole + 288u] = 16u;
        runtime.bytes[0xafffu] = reference;
        Check(pal4::CombatConsoleWindow::Destruct(kConsole, runtime) ==
                  0xdeadbeefu &&
                  runtime.words[kConsole] == pal4::CombatConsoleWindow::kVtable &&
                  runtime.words[kConsole + 40u] ==
                      pal4::CombatConsoleWindow::kFieldVtable &&
                  runtime.words[kConsole + 296u] == 0u &&
                  runtime.words[kConsole + 300u] == 0u &&
                  runtime.words[kConsole + 304u] == 0u &&
                  runtime.words[kConsole + 280u] == 0u &&
                  runtime.words[kConsole + 284u] == 0u &&
                  runtime.words[kConsole + 288u] == 0u,
              "destructor clears members and returns base result");
        Check(runtime.calls ==
                  (reference == 1u
                       ? std::vector<std::string>{
                             "vtable", "field-vtable", "release-entry",
                             "release-entry", "release-entry", "free:40960",
                             "decrement-reference", "base-destruct"}
                       : std::vector<std::string>{
                             "vtable", "field-vtable", "release-entry",
                             "release-entry", "release-entry", "free:40960",
                             "free:45055", "base-destruct"}),
              "array and reference cleanup order");
        Check(runtime.freed ==
                  (reference == 1u ? std::vector<std::uint32_t>{0xa000u}
                                   : std::vector<std::uint32_t>{
                                         0xa000u, 0xafffu}),
              "reference-counted text free branches");
        if (reference == 1u)
            Check(runtime.bytes[0xafffu] == 0u,
                  "shared text reference decremented");
    }

    Runtime empty;
    empty.words[kConsole + 296u] = 0u;
    empty.words[kConsole + 300u] = 0u;
    empty.words[kConsole + 304u] = 0u;
    empty.words[kConsole + 280u] = 0u;
    (void)pal4::CombatConsoleWindow::Destruct(kConsole, empty);
    Check(empty.freed == std::vector<std::uint32_t>{0u} &&
              std::find(empty.calls.begin(), empty.calls.end(),
                        "release-entry") == empty.calls.end(),
          "empty array and null text path");
}

}  // namespace

int main() {
    try {
        ConstructionFieldsAndGeometry();
        AllIconRecords();
        DestructionAndReferenceCounts();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
