#include "pal4/pal_console_puzzle_command.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalConsolePuzzleRuntime {
    bool found = false;
    bool fail_add = false;
    std::vector<std::string> calls;
    std::vector<std::string> attributes;
    std::string output;

    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x1000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view format) override {
        Check(console == 0x1000u, "output console");
        calls.emplace_back("output");
        output = format;
        return -9;
    }
    void ClearConsole(std::uint32_t console) override {
        Check(console == 0x1000u, "clear console address");
        calls.emplace_back("clear");
    }
    std::uint32_t InitializeXmlDocument() override {
        calls.emplace_back("document");
        return 0x2000u;
    }
    std::optional<std::uint32_t> FindTreeAttribute(
        std::uint32_t document, std::string_view key) override {
        Check(document == 0x2000u && (key == "Mode" || key == "Type"),
              "tree attribute operands");
        calls.emplace_back("find " + std::string(key));
        return found ? std::optional<std::uint32_t>(0x3000u)
                     : std::nullopt;
    }
    std::optional<std::uint32_t> FindConfigAttribute(
        std::uint32_t, std::string_view) override {
        throw std::runtime_error("puzzle must use tree lookup");
    }
    void SetInt(std::uint32_t attribute, std::int32_t value) override {
        Check(attribute == 0x3000u, "existing attribute address");
        calls.emplace_back("set " + std::to_string(value));
    }
    void SetString(std::uint32_t, std::string_view) override {
        throw std::runtime_error("puzzle only stores integers");
    }
    std::uint32_t InitializeIntAttribute(
        std::string_view key, std::int32_t value) override {
        attributes.emplace_back(std::string(key) + "=" +
                                std::to_string(value));
        calls.emplace_back("init " + std::string(key));
        return 0x4000u;
    }
    std::uint32_t InitializeStringAttribute(
        std::string_view, std::string_view) override {
        throw std::runtime_error("puzzle only stores integers");
    }
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override {
        Check(document == 0x2000u && attribute == 0x4000u,
              "add attribute operands");
        calls.emplace_back("add");
        if (fail_add) throw std::runtime_error("add failed");
    }
    void DestroyAttribute(std::uint32_t attribute) override {
        Check(attribute == 0x4000u, "temporary attribute address");
        calls.emplace_back("destroy attribute");
    }
    std::uint32_t GameInstance() override {
        calls.emplace_back("game");
        return 0x5000u;
    }
    void InitializeStateAndUi(std::uint32_t game,
                              std::uint32_t state_id,
                              std::uint32_t document) override {
        Check(game == 0x5000u && state_id == 8u && document == 0x2000u,
              "puzzle state operands");
        calls.emplace_back("state 8");
    }
    std::int32_t DestroyXmlDocument(std::uint32_t document) override {
        Check(document == 0x2000u, "destroy document address");
        calls.emplace_back("destroy document");
        return 31;
    }
};

void NewAndExistingAttributes() {
    Runtime fresh;
    Check(pal4::PalConsolePuzzleCommand::Execute("10 extra", fresh) == 31 &&
              fresh.attributes ==
                  std::vector<std::string>{"Mode=0", "Type=10"} &&
              fresh.calls == std::vector<std::string>{
                  "document", "find Mode", "init Mode", "add",
                  "destroy attribute", "find Type", "init Type", "add",
                  "destroy attribute", "console", "clear", "game",
                  "state 8", "destroy document"},
          "new puzzle attributes and state order");
    Runtime existing;
    existing.found = true;
    Check(pal4::PalConsolePuzzleCommand::Execute("-3", existing) == 31 &&
              existing.attributes.empty() &&
              existing.calls == std::vector<std::string>{
                  "document", "find Mode", "set 0", "find Type",
                  "set -3", "console", "clear", "game", "state 8",
                  "destroy document"},
          "negative type is accepted and existing attributes update");
}

void ArgumentErrors() {
    Runtime missing;
    Check(pal4::PalConsolePuzzleCommand::Execute(nullptr, missing) == -9 &&
              missing.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
              missing.calls == std::vector<std::string>{"console", "output"},
          "missing type uses original parameter error");
    Runtime excess;
    Check(pal4::PalConsolePuzzleCommand::Execute("11", excess) == -9 &&
              excess.output ==
                  "\xd7\xd3\xd3\xce\xcf\xb7\xc0\xe0\xd0\xcd\xb3\xac\xb9\xfd\xc9\xcf\xcf\xde" &&
              excess.calls == std::vector<std::string>{"console", "output"},
          "type 11 uses original upper-bound error");
    Runtime empty;
    Check(pal4::PalConsolePuzzleCommand::Execute("", empty) == 31 &&
              empty.attributes[1] == "Type=0",
          "atoi empty field becomes valid type zero");
}

void AddFailureCleansUp() {
    Runtime runtime;
    runtime.fail_add = true;
    try {
        (void)pal4::PalConsolePuzzleCommand::Execute("1", runtime);
        throw std::runtime_error("expected failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "add failed",
              "original add failure propagates");
    }
    Check(runtime.calls == std::vector<std::string>{
              "document", "find Mode", "init Mode", "add",
              "destroy attribute", "destroy document"},
          "exception destroys temporary attribute and document");
}

void Jigsaw() {
    Runtime upper;
    Check(pal4::PalConsolePuzzleCommand::StartJigsaw("12", upper) == 31 &&
              upper.attributes ==
                  std::vector<std::string>{"Mode=1", "Type=12"} &&
              upper.calls.back() == "destroy document",
          "jigsaw accepts type 12 and sets mode one");
    Runtime invalid;
    Check(pal4::PalConsolePuzzleCommand::StartJigsaw("13", invalid) ==
              -9 &&
              invalid.output ==
                  "\xd7\xd3\xd3\xce\xcf\xb7\xc0\xe0\xd0\xcd\xb3\xac\xb9\xfd\xc9\xcf\xcf\xde" &&
              invalid.calls == std::vector<std::string>{"console", "output"},
          "jigsaw rejects type 13 before document creation");
    Runtime negative;
    negative.found = true;
    Check(pal4::PalConsolePuzzleCommand::StartJigsaw("-1", negative) ==
              31 &&
              negative.calls == std::vector<std::string>{
                  "document", "find Mode", "set 1", "find Type",
                  "set -1", "console", "clear", "game", "state 8",
                  "destroy document"},
          "jigsaw shares signed type and existing-attribute path");
    Runtime missing;
    Check(pal4::PalConsolePuzzleCommand::StartJigsaw(nullptr, missing) ==
              -9 &&
              missing.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd",
          "jigsaw missing argument uses original error");
}

}  // namespace

int main() {
    try {
        NewAndExistingAttributes();
        ArgumentErrors();
        AddFailureCleansUp();
        Jigsaw();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
