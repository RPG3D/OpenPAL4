#include "pal4/pal_console_arena_load_command.h"

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

struct Entry {
    std::string key;
    std::string value;
    bool integer;
};

struct Runtime final : pal4::IPalConsoleArenaLoadRuntime {
    bool found = false;
    bool fail_add = false;
    std::vector<std::string> calls;
    std::vector<Entry> entries;
    std::string output;

    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x7000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view format) override {
        Check(console == 0x7000u, "usage console");
        calls.emplace_back("output");
        output = format;
        return -10;
    }
    std::uint32_t InitializeXmlDocument() override {
        calls.emplace_back("document");
        return 0x2000u;
    }
    std::optional<std::uint32_t> FindTreeAttribute(
        std::uint32_t document, std::string_view key) override {
        Check(document == 0x2000u, "tree document");
        calls.emplace_back("tree:" + std::string(key));
        return found ? std::optional<std::uint32_t>(0x3000u)
                     : std::nullopt;
    }
    std::optional<std::uint32_t> FindConfigAttribute(
        std::uint32_t document, std::string_view key) override {
        Check(document == 0x2000u, "config document");
        calls.emplace_back("config:" + std::string(key));
        return found ? std::optional<std::uint32_t>(0x3000u)
                     : std::nullopt;
    }
    void SetInt(std::uint32_t attribute, std::int32_t value) override {
        Check(attribute == 0x3000u, "integer attribute address");
        calls.emplace_back("set-int:" + std::to_string(value));
    }
    void SetString(std::uint32_t attribute,
                   std::string_view value) override {
        Check(attribute == 0x3000u, "string attribute address");
        calls.emplace_back("set-string:" + std::string(value));
    }
    std::uint32_t InitializeIntAttribute(
        std::string_view key, std::int32_t value) override {
        entries.push_back({std::string(key), std::to_string(value), true});
        calls.emplace_back("init-int");
        return 0x4000u;
    }
    std::uint32_t InitializeStringAttribute(
        std::string_view key, std::string_view value) override {
        entries.push_back({std::string(key), std::string(value), false});
        calls.emplace_back("init-string");
        return 0x4000u;
    }
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override {
        Check(document == 0x2000u && attribute == 0x4000u,
              "new attribute operands");
        calls.emplace_back("add");
        if (fail_add) throw std::runtime_error("add failed");
    }
    void DestroyAttribute(std::uint32_t attribute) override {
        Check(attribute == 0x4000u, "temporary attribute cleanup");
        calls.emplace_back("destroy-attribute");
    }
    std::uint32_t GameInstance() override {
        calls.emplace_back("game");
        return 0x5000u;
    }
    void InitializeStateAndUi(std::uint32_t game,
                              std::uint32_t state_id,
                              std::uint32_t document) override {
        Check(game == 0x5000u && state_id == 6u && document == 0x2000u,
              "loading-state operands");
        calls.emplace_back("state");
    }
    std::int32_t DestroyXmlDocument(std::uint32_t document) override {
        Check(document == 0x2000u, "document cleanup address");
        calls.emplace_back("destroy-document");
        return 47;
    }
};

void MissingParameters() {
    for (const char* input :
         std::vector<const char*>{nullptr, "arena", "arena area"}) {
        Runtime runtime;
        Check(pal4::PalConsoleArenaLoadCommand::Execute(
                  input, runtime) == -10 &&
                  runtime.calls == std::vector<std::string>{
                      "console", "output"} &&
                  runtime.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd",
              "missing field reports GBK error before document creation");
    }
}

void InsertAttributesInIdaOrder() {
    Runtime runtime;
    Check(pal4::PalConsoleArenaLoadCommand::Execute(
              "arenaA areaB resC extra", runtime) == 47,
          "document destructor result");
    Check(runtime.entries.size() == 5u &&
              runtime.entries[0].key == "PALGAMEIV_LOADING_TYPE" &&
              runtime.entries[0].value == "0" &&
              runtime.entries[0].integer &&
              runtime.entries[1].key == "PALGAMEIV_LOADING_ARENA" &&
              runtime.entries[1].value == "arenaA" &&
              !runtime.entries[1].integer &&
              runtime.entries[2].key == "PALGAMEIV_LOADING_AREA" &&
              runtime.entries[2].value == "areaB" &&
              runtime.entries[3].key == "PALGAMEIV_LOADING_AREA_RES" &&
              runtime.entries[3].value == "resC" &&
              runtime.entries[4].key == "PALGAMEIV_LOADING_AUTOREADY" &&
              runtime.entries[4].value == "1" &&
              runtime.entries[4].integer,
          "five native XML keys, types and values");
    Check(runtime.calls == std::vector<std::string>{
              "document", "tree:PALGAMEIV_LOADING_TYPE", "init-int",
              "add", "destroy-attribute",
              "tree:PALGAMEIV_LOADING_ARENA", "init-string",
              "add", "destroy-attribute",
              "tree:PALGAMEIV_LOADING_AREA", "init-string",
              "add", "destroy-attribute",
              "config:PALGAMEIV_LOADING_AREA_RES", "init-string",
              "add", "destroy-attribute",
              "config:PALGAMEIV_LOADING_AUTOREADY", "init-int",
              "add", "destroy-attribute", "game", "state",
              "destroy-document"},
          "lookup, insertion, state and release order");
}

void UpdateExistingAttributes() {
    Runtime runtime;
    runtime.found = true;
    Check(pal4::PalConsoleArenaLoadCommand::Execute(
              "arena area resources", runtime) == 47 &&
              runtime.entries.empty() &&
              runtime.calls == std::vector<std::string>{
                  "document", "tree:PALGAMEIV_LOADING_TYPE", "set-int:0",
                  "tree:PALGAMEIV_LOADING_ARENA", "set-string:arena",
                  "tree:PALGAMEIV_LOADING_AREA", "set-string:area",
                  "config:PALGAMEIV_LOADING_AREA_RES",
                  "set-string:resources",
                  "config:PALGAMEIV_LOADING_AUTOREADY", "set-int:1",
                  "game", "state", "destroy-document"},
          "existing attributes update in place");
}

void AddFailureReleasesResources() {
    Runtime runtime;
    runtime.fail_add = true;
    try {
        (void)pal4::PalConsoleArenaLoadCommand::Execute(
            "arena area resources", runtime);
        throw std::runtime_error("expected add failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "add failed",
              "original add failure propagates");
    }
    Check(runtime.calls == std::vector<std::string>{
              "document", "tree:PALGAMEIV_LOADING_TYPE", "init-int",
              "add", "destroy-attribute", "destroy-document"},
          "exception destroys temporary attribute and document");
}

}  // namespace

int main() {
    try {
        MissingParameters();
        InsertAttributesInIdaOrder();
        UpdateExistingAttributes();
        AddFailureReleasesResources();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
