#include "pal4/pal_console_player_attribute_commands.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalConsolePlayerAttributeRuntime {
    std::vector<std::string> calls;
    std::int32_t id = 2;
    std::uint32_t player = 0x2000u;
    std::uint32_t attribute = 0x4000u;
    pal4::PalPlayerAttributeKind kind = pal4::PalPlayerAttributeKind::Player;
    std::string selected_name;
    std::string property = "25";
    std::string assigned;
    std::string output;
    bool fail_output = false;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x1000u;
    }
    std::int32_t PlayerId(std::uint32_t manager,
                          std::string_view name) override {
        Check(manager == 0x1000u, "player manager for ID");
        selected_name = name;
        calls.emplace_back("id");
        return id;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x1000u && index == id, "player lookup operands");
        calls.emplace_back("player");
        return player;
    }
    std::uint32_t AttributeCollection(
        pal4::PalPlayerAttributeKind selected_kind) override {
        kind = selected_kind;
        calls.emplace_back("collection");
        return selected_kind == pal4::PalPlayerAttributeKind::Player ?
                   0x3000u : 0x3100u;
    }
    std::uint32_t FindAttribute(std::uint32_t collection,
                                std::string_view name) override {
        Check(collection == (kind == pal4::PalPlayerAttributeKind::Player ?
                                 0x3000u : 0x3100u),
              "attribute collection");
        selected_name = name;
        calls.emplace_back("find");
        return attribute;
    }
    void CheckContext(const pal4::PalPlayerAttributeContext& context,
                      std::uint32_t descriptor) {
        Check(context.vtable == 0x845560u && context.reserved == 0u &&
                  descriptor == attribute &&
                  context.object == player +
                      (kind == pal4::PalPlayerAttributeKind::Basic ? 0x658u : 0u),
              "attribute context and descriptor");
    }
    void SetAttribute(const pal4::PalPlayerAttributeContext& context,
                      std::uint32_t descriptor,
                      std::string_view value) override {
        CheckContext(context, descriptor);
        calls.emplace_back("set");
        assigned = value;
        property = assigned;
    }
    std::string_view GetAttribute(
        const pal4::PalPlayerAttributeContext& context,
        std::uint32_t descriptor) override {
        CheckContext(context, descriptor);
        calls.emplace_back("get");
        return property;
    }
    std::string_view PlayerName(std::uint32_t subject) override {
        Check(subject == player, "player name at player + 4");
        calls.emplace_back("name");
        return "Yun";
    }
    void InitializeFormatBuffer(std::string_view name) override {
        Check(name == "Yun", "format buffer initial value");
        calls.emplace_back("init buffer");
    }
    std::int32_t CleanupFormatBuffer() override {
        calls.emplace_back("cleanup buffer");
        return 57;
    }
    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x5000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view text) override {
        Check(console == 0x5000u, "console address");
        calls.emplace_back("output");
        if (fail_output) throw std::runtime_error("output failed");
        output = text;
        return -7;
    }
};

using Command = std::int32_t (*)(
    const char*, pal4::IPalConsolePlayerAttributeRuntime&);

constexpr std::array<Command, 4> kCommands{
    pal4::PalConsolePlayerAttributeCommands::GetPlayer,
    pal4::PalConsolePlayerAttributeCommands::SetPlayer,
    pal4::PalConsolePlayerAttributeCommands::GetBasic,
    pal4::PalConsolePlayerAttributeCommands::SetBasic};

void SuccessfulCommands() {
    for (std::size_t index = 0; index < kCommands.size(); ++index) {
        Runtime runtime;
        const bool write = index == 1u || index == 3u;
        const bool basic = index >= 2u;
        const char* input = write ? "hero hp -19" : "hero hp";
        const auto result = kCommands[index](input, runtime);
        std::vector<std::string> expected{
            "manager", "id", "manager", "player", "collection", "find"};
        if (write) expected.emplace_back("set");
        expected.insert(expected.end(), {"get", "name", "init buffer",
                                        "console", "output", "cleanup buffer"});
        Check(result == 57 && runtime.calls == expected &&
                  runtime.kind == (basic ? pal4::PalPlayerAttributeKind::Basic :
                                           pal4::PalPlayerAttributeKind::Player) &&
                  runtime.selected_name == "hp" &&
                  runtime.output == (write ? "Yun hp :-19" : "Yun hp :25") &&
                  (!write || runtime.assigned == "-19"),
              "attribute command order, context, output and cleanup");
    }
}

void ErrorBranches() {
    for (std::size_t index = 0; index < kCommands.size(); ++index) {
        const bool write = index == 1u || index == 3u;
        for (const char* input : std::array<const char*, 3>{
                 nullptr, "hero", write ? "hero hp" : ""}) {
            Runtime runtime;
            Check(kCommands[index](input, runtime) == -7 &&
                      runtime.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
                      runtime.calls == std::vector<std::string>{"console", "output"},
                  "missing argument error");
        }
        Runtime invalid;
        invalid.id = 4;
        Check(kCommands[index](write ? "hero hp 1" : "hero hp", invalid) == -7 &&
                  invalid.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
                  invalid.calls == std::vector<std::string>{
                      "manager", "id", "console", "output"},
              "player ID four error");
        Runtime absent;
        absent.player = 0u;
        Check(kCommands[index](write ? "hero hp 1" : "hero hp", absent) == 0 &&
                  absent.calls == std::vector<std::string>{
                      "manager", "id", "manager", "player"},
              "null player exits without output");
        Runtime unknown;
        unknown.attribute = 0u;
        Check(kCommands[index](write ? "hero hp 1" : "hero hp", unknown) == -7 &&
                  unknown.output ==
                      "\xb8\xc3\xca\xf4\xd0\xd4\xb2\xbb\xb4\xe6\xd4\xda" &&
                  unknown.calls == std::vector<std::string>{
                      "manager", "id", "manager", "player", "collection",
                      "find", "console", "output"},
              "missing attribute error");
    }
    Runtime empty_value;
    Check(pal4::PalConsolePlayerAttributeCommands::SetPlayer(
              "hero hp ", empty_value) == 57 &&
              empty_value.assigned.empty() && empty_value.output == "Yun hp :",
          "empty third field remains a valid parser result");
}

void OutputFailureCleansBuffer() {
    Runtime runtime;
    runtime.fail_output = true;
    try {
        (void)pal4::PalConsolePlayerAttributeCommands::SetBasic(
            "hero hp 1", runtime);
        throw std::runtime_error("expected output failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "output failed",
              "output exception propagation");
    }
    Check(runtime.calls == std::vector<std::string>{
              "manager", "id", "manager", "player", "collection", "find",
              "set", "get", "name", "init buffer", "console", "output",
              "cleanup buffer"},
          "initialized buffer released on output failure");
}

}  // namespace

int main() {
    try {
        SuccessfulCommands();
        ErrorBranches();
        OutputFailureCleansBuffer();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
